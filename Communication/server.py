import socket, json, time, sqlite3, logging, os, sys
from daemon_base import Daemon

# 설정 파일 로드 (Windows 테스트 시 사용)
# with open("/home/hyunmin/SixWheelBot/Communication/config/comm_config.json", "r", encoding="utf-8") as f:
# with open("C:\\vscode\\SixWheelBot\\Communication\\config\\config.json", "r", encoding="utf-8") as f:
#     config = json.load(f)

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
CONFIG_PATH = os.path.join(BASE_DIR, 'config', 'config.json')
with open(CONFIG_PATH, 'r', encoding='utf-8') as f:
    config = json.load(f)

# window 환경에서는 그대로, linux 환경에서는 주석 부분을 사용
srv_conf = config["SERVER"]
cli_conf = config["CLIENT"]
net_conf = config["NETWORK"]
log_conf = config["WIN_LOG"]
db_conf  = config["WIN_DB"]
# log_conf = config["LOG"]
# db_conf  = config["DB"]

SERVER_IP   = srv_conf["IP"]
SERVER_PORT = int(srv_conf["PORT"])
CLIENT_IP   = cli_conf["IP"]
CLIENT_PORT = int(cli_conf["PORT"])
ALLOW_IP    = net_conf["ALLOW_IP"]
ACK_TIMEOUT = float(net_conf["ACK_TIMEOUT"])
RETRY_LIMIT = int(net_conf["RETRY_LIMIT"])
LOG_MODE    = log_conf["LOG_MODE"].upper()
LOG_FILE    = log_conf["SERVER_LOG_FILE"]
DB_FILE     = config["DB"]["ROBOT_DB_FILE"]
MAP_FILE    = config["WIN_MAP"]["MAP_FILE"]
# MAP_FILE    = config["MAP"]["MAP_FILE"]

logger = logging.getLogger("ServerDaemon")
logger.setLevel(getattr(logging, LOG_MODE))
fh = logging.FileHandler(LOG_FILE, mode="a+", encoding="utf-8")
fh.setFormatter(logging.Formatter("%(asctime)s [%(levelname)s] %(message)s"))
logger.addHandler(fh)

def save_to_db(data):
    conn = sqlite3.connect(DB_FILE)
    cur = conn.cursor()
    cur.execute('''
        CREATE TABLE IF NOT EXISTS robot_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp REAL,
            state TEXT,
            lat REAL,
            lon REAL
        )
    ''')
    cur.execute(
        'INSERT INTO robot_logs (timestamp, state, lat, lon) VALUES (?, ?, ?, ?)',
        (data['timestamp'], data['robot_state'], data['gps']['lat'], data['gps']['lon'])
    )
    conn.commit()
    conn.close()

# mapdata.txt 로드 함수
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

def server_loop():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((SERVER_IP, SERVER_PORT))
    logger.info(f"서버 바인드 완료: {SERVER_IP}:{SERVER_PORT}")
    sock.settimeout(ACK_TIMEOUT)

    # 캐시 구조
    map_cache = {}
    done_cache = {}
    log_cache = {}
    received_logs = set()

    # 1) map 전송 & ACK_MAP
    route = load_map_data(MAP_FILE)
    map_pkt = {'packet_number': 0, 'type': 'map', 'route': route, 'timestamp': time.time()}
    data_map = json.dumps(map_pkt).encode()
    map_cache[0] = data_map

    for attempt in range(RETRY_LIMIT):
        sock.sendto(data_map, (CLIENT_IP, CLIENT_PORT))
        logger.info(f'Map sent attempt {attempt+1}')
        try:
            data, addr = sock.recvfrom(1024)
            if data.decode() == 'ACK_MAP:0':
                logger.info('ACK_MAP received')
                del map_cache[0]
                break
        except socket.timeout:
            continue
    else:
        logger.error('Map ACK not received after retries')

    # 2) 로그 수신
    while True:
        try:
            data, addr = sock.recvfrom(65536)
        except socket.timeout:
            continue
        msg = data.decode(errors='ignore').strip()

        # 재전송 요청 처리
        if msg.startswith('RETRANS_MAP:'):
            sock.sendto(map_cache.get(0, b''), (CLIENT_IP, CLIENT_PORT))
            logger.info('Retransmitted map')
            continue
        if msg.startswith('RETRANS_LOG:'):
            num = int(msg.split(':')[1])
            sock.sendto(log_cache.get(num, b''), addr)
            logger.info(f'Retransmitted log {num}')
            continue
        if msg.startswith('RETRANS_DONE:'):
            num = int(msg.split(':')[1])
            sock.sendto(done_cache.get(num, b''), addr)
            logger.info(f'Retransmitted done {num}')
            continue

        if not msg.startswith('{'):
            continue
        pkt = json.loads(msg); ptype = pkt.get('type'); num = pkt.get('packet_number')

        # log 처리
        if ptype == 'log':
            if num in received_logs:
                sock.sendto(f'ACK_LOG:{num}'.encode(), addr)
                continue
            save_to_db(pkt)
            data_log = json.dumps(pkt).encode()
            log_cache[num] = data_log
            sock.sendto(f'ACK_LOG:{num}'.encode(), addr)
            received_logs.add(num)
            logger.info(f'Log {num} saved & ACK_LOG')
        # done 처리
        elif ptype == 'done':
            data_done = json.dumps(pkt).encode()
            done_cache[num] = data_done
            for attempt in range(RETRY_LIMIT):
                sock.sendto(data_done, addr)
                logger.info(f'Done {num} sent attempt {attempt+1}')
                try:
                    ack, _ = sock.recvfrom(1024)
                    if ack.decode() == f'ACK_DONE:{num}':
                        del done_cache[num]
                        logger.info('ACK_DONE received')
                        break
                except socket.timeout:
                    continue
            break


class ServerDaemon(Daemon):
    def run(self):
        server_loop()

if __name__ == "__main__":
    import platform
    # Windows 에서 테스트용 실행
    if platform.system().lower().startswith("win"):
        print("== Windows Foreground 모드로 서버 실행 ==")
        logger.info(f'Server Start')
        server_loop()
    else:
        # Linux/Unix 데몬 제어
        pidf   = "/var/run/server.pid"
        daemon = ServerDaemon(pidf, stdout=LOG_FILE, stderr=LOG_FILE)
        if len(sys.argv) == 2:
            cmd = sys.argv[1]
            if cmd == "start":   
                logger.info(f'Server Daemon {cmd}')
                daemon.start()
            elif cmd == "stop":    
                logger.info(f'Server Daemon {cmd}')
                daemon.stop()
            elif cmd == "status":  
                daemon.status()
            elif cmd == "restart": 
                logger.info(f'Server Daemon {cmd}')
                daemon.restart()
            else: print("Usage: server.py [start|stop|status|restart]")
        else:
            print("Usage: server.py [start|stop|status|restart]")
