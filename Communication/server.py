import socket, json, time, sqlite3, logging, os, sys
from daemon_base import Daemon

# 설정 파일 로드 (Windows 테스트 시 사용)
# with open("/home/hyunmin/SixWheelBot/Communication/config/comm_config.json", "r", encoding="utf-8") as f:
# with open("C:\\vscode\\SixWheelBot\\Communication\\config\\config.json", "r", encoding="utf-8") as f:
#     config = json.load(f)

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
CONFIG_PATH = os.path.join(BASE_DIR, 'config', 'comm_config.json')
with open(CONFIG_PATH, 'r', encoding='utf-8') as f:
    config = json.load(f)

srv_conf = config["SERVER"]
log_conf = config["LOG"]
db_conf  = config["DB"]

SERVER_IP   = srv_conf["SERVER_IP"]
SERVER_PORT = int(srv_conf["SERVER_PORT"])
CLIENT_IP   = srv_conf["CLIENT_IP"]
CLIENT_PORT = int(srv_conf["CLIENT_PORT"])
ACK_TIMEOUT = float(srv_conf["ACK_TIMEOUT"])
LOG_MODE    = log_conf["LOG_MODE"].upper()
LOG_FILE    = log_conf["SERVER_LOG_FILE"]
DB_FILE     = config["DB"]["ROBOT_DB_FILE"]

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

def server_loop():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    # sock.bind((SERVER_IP, SERVER_PORT))
    # logger.info(f"서버 바인드 완료: {SERVER_IP}:{SERVER_PORT}")
    sock.settimeout(ACK_TIMEOUT)

    # 캐시 구조
    cmd_cache = {}
    log_cache = {}
    received_logs = set()

    # 1) 명령 생성 및 캐싱
    cmd_num = 0
    cmd = {
        'packet_number': cmd_num,
        'type': 'command',
        'route': [
            {'lat': 37.123, 'lon': 127.123},
            {'lat': 37.124, 'lon': 127.124},
            {'lat': 37.125, 'lon': 127.125}
        ],
        'timestamp': time.time()
    }
    data_cmd = json.dumps(cmd).encode()
    cmd_cache[cmd_num] = data_cmd

    # 2) 명령 전송 및 ACK_CMD 대기
    while True:
        sock.sendto(data_cmd, (CLIENT_IP, CLIENT_PORT))
        logger.info(f'Command {cmd_num} sent')
        data, addr = sock.recvfrom(1024)
        msg = data.decode()
        if msg == f'ACK_CMD:{cmd_num}':
            logger.info(f'Command {cmd_num} ack received')
            del cmd_cache[cmd_num]
            break
        time.sleep(1)

    # 3) 로그 수신 및 ACK_LOG 처리
    while True:
        data, addr = sock.recvfrom(65536)
        msg = data.decode(errors='ignore').strip()

        # 재전송 요청 처리
        if msg.startswith('RETRANS_CMD:'):
            req = int(msg.split(':')[1])
            if req in cmd_cache:
                sock.sendto(cmd_cache[req], (CLIENT_IP, CLIENT_PORT))
                logger.info(f'Retransmitted command {req}')
            continue
        if msg.startswith('RETRANS_LOG:'):
            req = int(msg.split(':')[1])
            if req in log_cache:
                sock.sendto(log_cache[req], (CLIENT_IP, CLIENT_PORT))
                logger.info(f'Retransmitted log {req}')
            continue

        if not msg.startswith('{'):
            continue
        pkt = json.loads(msg)
        ptype = pkt.get('type')
        num = pkt.get('packet_number')

        # 중복 로그 재전송 ACK
        if num in received_logs:
            sock.sendto(f'ACK_LOG:{num}'.encode(), addr)
            continue

        if ptype == 'log':
            save_to_db(pkt)
            log_cache[num] = msg.encode()
            sock.sendto(f'ACK_LOG:{num}'.encode(), addr)
            received_logs.add(num)
            logger.info(f'Log {num} processed and ACK_LOG sent')
        elif ptype == 'done':
            sock.sendto(f'ACK_DONE:{num}'.encode(), addr)
            logger.info(f'Done {num} received and ACK_DONE sent')
            break
        else:
            logger.warning(f'Unexpected packet type: {ptype}')

class ServerDaemon(Daemon):
    def run(self):
        server_loop()

if __name__ == "__main__":
    import platform
    # Windows 에서 테스트용 실행
    if platform.system().lower().startswith("win"):
        print("== Windows Foreground 모드로 서버 실행 ==")
        server_loop()
    else:
        # Linux/Unix 데몬 제어
        pidf   = "/var/run/server.pid"
        daemon = ServerDaemon(pidf, stdout=LOG_FILE, stderr=LOG_FILE)
        if len(sys.argv) == 2:
            cmd = sys.argv[1]
            if cmd == "start":   daemon.start()
            elif cmd == "stop":    daemon.stop()
            elif cmd == "status":  daemon.status()
            elif cmd == "restart": daemon.restart()
            else: print("Usage: server.py [start|stop|status|restart]")
        else:
            print("Usage: server.py [start|stop|status|restart]")
