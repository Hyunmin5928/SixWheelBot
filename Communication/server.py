import socket, json, time, sqlite3, logging, os, sys
from daemon_base import Daemon

BASE_DIR    = os.path.dirname(os.path.abspath(__file__))
CONFIG_PATH = os.path.join(BASE_DIR, 'config', 'config.json')
with open(CONFIG_PATH, 'r', encoding='utf-8') as f:
    config = json.load(f)

# Windows 테스트용 설정
srv_conf  = config["SERVER"]
cli_conf  = config["CLIENT"]
net_conf  = config["NETWORK"]
log_conf  = config["WIN_LOG"]
db_conf   = config["WIN_DB"]
MAP_FILE  = config["WIN_MAP"]["MAP_FILE"]

# 실제 Linux 환경용(주석 해제)
# log_conf  = config["LOG"]
# db_conf   = config["DB"]
# MAP_FILE  = config["MAP"]["MAP_FILE"]

SERVER_IP   = srv_conf["IP"]
SERVER_PORT = int(srv_conf["PORT"])
CLIENT_IP   = cli_conf["IP"]
CLIENT_PORT = int(cli_conf["PORT"])
ALLOW_IP    = net_conf["ALLOW_IP"]
ACK_TIMEOUT = float(net_conf["ACK_TIMEOUT"])
RETRY_LIMIT = int(net_conf["RETRY_LIMIT"])
LOG_FILE    = log_conf["SERVER_LOG_FILE"]
DB_FILE     = config["DB"]["ROBOT_DB_FILE"]

# 로거 설정
logger = logging.getLogger("ServerDaemon")
logger.setLevel(getattr(logging, log_conf["LOG_MODE"].upper()))
fh = logging.FileHandler(LOG_FILE, mode="a+", encoding="utf-8")
fh.setFormatter(logging.Formatter("%(asctime)s [%(levelname)s] %(message)s"))
logger.addHandler(fh)


def save_to_db(data):
    conn = sqlite3.connect(DB_FILE)
    cur  = conn.cursor()
    # cur.execute('''
    #     CREATE TABLE IF NOT EXISTS robot_logs (
    #         id        INTEGER PRIMARY KEY AUTOINCREMENT,
    #         timestamp REAL,
    #         state     TEXT,
    #         lat       REAL,
    #         lon       REAL
    #     )
    # ''')
    # cur.execute(
    #     'INSERT INTO robot_logs (timestamp, state, lat, lon) VALUES (?, ?, ?, ?)',
    #     (data['timestamp'], data['robot_state'],
    #      data['gps']['lat'], data['gps']['lon'])
    # )
    conn.commit()
    conn.close()


def load_map_data(path):
    route = []
    with open(path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            lat_str, lon_str, dir_str = line.split(',', 2)
            try:
                lat = float(lat_str); lon = float(lon_str); dir = int(dir_str)
                logger.info(f"lat : {lat}, lon : {lon}, dir : {dir}")
            except ValueError:
                continue
            route.append({'lat': lat, 'lon': lon, 'dir' : dir})
    return route


def distance_m(a, b):
    dx = (a[0] - b[0]) * 111000.0
    dy = (a[1] - b[1]) * 111000.0
    return (dx*dx + dy*dy) ** 0.5


def server_loop():
    # 1) 소켓 생성 및 바인드
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((ALLOW_IP, SERVER_PORT))
    logger.info(f"서버 바인드 완료: {ALLOW_IP}:{SERVER_PORT}")
    sock.settimeout(ACK_TIMEOUT)

    # 2) Map 전송 & ACK_MAP
    route = load_map_data(MAP_FILE)
    map_pkt = {
        'packet_number': 0,
        'type': 'map',
        'route': route,
        'timestamp': time.time()
    }
    data_map = json.dumps(map_pkt).encode()
    for attempt in range(RETRY_LIMIT):
        sock.sendto(data_map, (CLIENT_IP, CLIENT_PORT))
        logger.info(f"Map sent attempt {attempt+1}")
        try:
            data, addr = sock.recvfrom(1024)
            if data.decode() == 'ACK_MAP:0':
                logger.info("ACK_MAP received")
                break
        except socket.timeout:
            continue
    else:
        logger.error("Map ACK not received after retries")
        # 실패 시에도 계속 대기하거나, exit 할 수 있음

    received_logs = set()
    destination   = (route[-1]['lat'], route[-1]['lon'])
    returned      = False

    # 3) 로그 수신 & 명령 전송 루프
    while True:
        try:
            data, addr = sock.recvfrom(65536)
        except socket.timeout:
            continue
        logger.info(f"Raw from {addr}: {data!r}")
        msg = data.decode(errors='ignore').strip()

        # Retrans 요청 처리
        if msg.startswith('RETRANS_MAP:'):
            sock.sendto(data_map, (CLIENT_IP, CLIENT_PORT))
            logger.info("Retransmitted map")
            continue

        if msg.startswith('RETRANS_LOG:'):
            num = int(msg.split(':')[1])
            sock.sendto(f"ACK_LOG:{num}".encode(), addr)
            logger.info(f"Retransmitted log {num}")
            continue

        if not msg.startswith('{'):
            continue

        pkt   = json.loads(msg)
        ptype = pkt.get('type')
        num   = pkt.get('packet_number')

        # -- 로그 처리 --
        if ptype == 'log':
            if num not in received_logs:
                save_to_db(pkt)
                received_logs.add(num)
                logger.info(f"Log {num} saved")
            sock.sendto(f"ACK_LOG:{num}".encode(), addr)

            # 위치 판정
            lat = pkt['gps']['lat']; lon = pkt['gps']['lon']
            dist = distance_m((lat, lon), destination)

            # 목적지 도달 시 pause→unlock→return 전송
            if not returned and dist <= 0.5:
                # 1) pause
                cmd = json.dumps({"type":"cmd","action":"pause"}).encode()
                sock.sendto(cmd, addr)
                logger.info("Pause command sent (destination reached)")
                time.sleep(1)

                # 2) unlock
                cmd = json.dumps({"type":"cmd","action":"unlock"}).encode()
                sock.sendto(cmd, addr)
                logger.info("Unlock command sent")
                time.sleep(2)

                # 3) return
                cmd = json.dumps({"type":"cmd","action":"return"}).encode()
                sock.sendto(cmd, addr)
                logger.info("Return command sent")
                returned = True

        # -- done 처리 --
        elif ptype == 'done':
            sock.sendto(f"ACK_DONE:{num}".encode(), addr)
            logger.info("ACK_DONE sent")

            # 복귀 완료 시 최종 pause 전송 후 종료
            if returned:
                cmd = json.dumps({"type":"cmd","action":"pause"}).encode()
                sock.sendto(cmd, addr)
                logger.info("Final pause command sent (return completed)")
                break   # 루프 탈출 → 소켓 닫기

    # 4) 복귀 완료 후에만 소켓 닫기
    sock.close()
    logger.info("Socket closed after return completed")


class ServerDaemon(Daemon):
    def run(self):
        server_loop()


if __name__ == "__main__":
    import platform
    if platform.system().lower().startswith("win"):
        print("== Windows Foreground 모드로 서버 실행 ==")
        logger.info("Server start (Windows)")
        server_loop()
    else:
        pid_file = "/var/run/server.pid"
        daemon   = ServerDaemon(pid_file,
                                stdout=LOG_FILE,
                                stderr=LOG_FILE)
        if len(sys.argv) == 2:
            cmd = sys.argv[1]
            if cmd == "start":
                logger.info("Server Daemon start")
                daemon.start()
            elif cmd == "stop":
                logger.info("Server Daemon stop")
                daemon.stop()
            elif cmd == "restart":
                logger.info("Server Daemon restart")
                daemon.restart()
            elif cmd == "status":
                daemon.status()
            else:
                print("Usage: server.py [start|stop|status|restart]")
        else:
            print("Usage: server.py [start|stop|status|restart]")
