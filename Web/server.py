import socket, json, time, threading, logging, os, sys
from daemon_base import Daemon

# ── 설정 로드 ────────────────────────────────────────────────────────────────
BASE_DIR    = os.path.dirname(os.path.abspath(__file__))
CONFIG_PATH = os.path.join(BASE_DIR, '../Config', 'config.json')
with open(CONFIG_PATH, 'r', encoding='utf-8') as f:
    config = json.load(f)

srv_conf   = config["SERVER"]
cli_conf   = config["CLIENT"]
net_conf   = config["NETWORK"]
log_conf   = config["LOG"]
db_conf    = config["DB"]
DB_FILE    = db_conf["DB_FILE"]
MAP_FILE   = config["MAP"]["MAP_FILE"]

SERVER_IP    = srv_conf["IP"]
SERVER_PORT  = int(srv_conf["PORT"])
CLIENT_IP    = cli_conf["IP"]
CLIENT_PORT  = int(cli_conf["PORT"])
LOCAL_HOST   = net_conf["LOCAL_HOST"]
CONTROL_PORT = 6001               # Node.js 제어 메시지 수신용 포트
ALLOW_IP     = net_conf["ALLOW_IP"]
ACK_TIMEOUT  = float(net_conf["ACK_TIMEOUT"])
RETRY_LIMIT  = int(net_conf["RETRY_LIMIT"])

# ── 로거 설정 ────────────────────────────────────────────────────────────────
logger = logging.getLogger("ServerDaemon")
level_map = {0: logging.DEBUG, 1: logging.INFO, 2: logging.WARNING, 3: logging.ERROR}
lvl = log_conf.get("LOG_LEVEL", 1)
logger.setLevel(level_map.get(lvl, logging.INFO))
fh = logging.FileHandler(log_conf["SERVER_LOG_FILE"], mode="a+", encoding="utf-8")
fh.setFormatter(logging.Formatter("%(asctime)s [%(levelname)s] %(message)s"))
logger.addHandler(fh)

# ── 맵 데이터 로드 함수 ───────────────────────────────────────────────────────
def load_map_data(path):
    route = []
    with open(path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'): continue
            lat, lon, dir_str = line.split(',', 2)
            try:
                route.append({'lat': float(lat), 'lon': float(lon), 'dir': int(dir_str)})
            except ValueError:
                continue
    return route

# ── 메인 데몬 클래스 ─────────────────────────────────────────────────────────
class DeliveryDaemon(Daemon):
    def run(self):
        # 소켓 생성 및 바인드 (run 시점에만 실행)
        self.robot_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.robot_sock.bind((ALLOW_IP, SERVER_PORT))
        self.robot_sock.settimeout(ACK_TIMEOUT)

        self.control_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.control_sock.bind((LOCAL_HOST, CONTROL_PORT))

        # 제어 리스너 시작
        threading.Thread(target=self.control_listener, daemon=True).start()

        # idle 대기
        while True:
            time.sleep(1)

    def control_listener(self):
        logger.info(f"Control listener started on {SERVER_IP}:{CONTROL_PORT}")
        while True:
            data, addr = self.control_sock.recvfrom(1024)
            msg = json.loads(data.decode())
            t   = msg.get('type')
            if t == 'start':
                logger.info("Start command received, launching server_loop thread")
                threading.Thread(target=self.server_loop, daemon=True).start()
            elif t in ('unlock','return','pause'):
                logger.info(f"Control command received: {t}")
                self.send_command(t)

    def server_loop(self):
        robot_sock = self.robot_sock
        destination = None

        # 1) Map 전송 & ACK_MAP
        route = load_map_data(MAP_FILE)
        destination = (route[-1]['lat'], route[-1]['lon'])
        map_pkt = {'packet_number': 0, 'type': 'map', 'route': route, 'timestamp': time.time()}
        data_map = json.dumps(map_pkt).encode()
        for attempt in range(RETRY_LIMIT):
            robot_sock.sendto(data_map, (CLIENT_IP, CLIENT_PORT))
            logger.info(f"Map sent attempt {attempt+1}")
            try:
                data, addr = robot_sock.recvfrom(1024)
                if data.decode() == 'ACK_MAP:0':
                    logger.info("ACK_MAP received")
                    break
            except socket.timeout:
                continue
        else:
            logger.error("Map ACK not received after retries")

        # 2) 로그 수신 & 자동 명령
        received = set()
        returned = False
        while True:
            # logger.info(f"Log recv sock activate")
            try:
                data, addr = robot_sock.recvfrom(65536)
            except socket.timeout:
                continue
            raw = data.decode(errors='ignore')
            logger.debug(f"RAW UDP from {addr}: {raw!r}")
            if not raw.startswith('{'):
                continue
            
            logger.info(f"Log recv sock recieved")
            pkt = json.loads(raw)
            ptype, num = pkt.get('type'), pkt.get('packet_number')
            if ptype == 'log':
                if num not in received:
                    received.add(num)
                    logger.info(f"Log {num} processed")
                robot_sock.sendto(f"ACK_LOG:{num}".encode(), addr)
                lat, lon = pkt['gps']['lat'], pkt['gps']['lon']
                # dist = ((lat-destination[0])**2 + (lon-destination[1])**2)**0.5 * 111000
                # if not returned and dist <= 0.5:
                    # for action in ('pause','unlock','return'):
                    #     cmd = json.dumps({'type':'cmd','action':action}).encode()
                    #     robot_sock.sendto(cmd, addr)
                    #     logger.info(f"{action} command sent (auto)")
                    #     time.sleep(1 if action!='unlock' else 2)
                    # returned = True
                logger.info(f"Packet data -> lat : {lat}, lon : {lon}")
                NODE_SERVER_ADDR = ('127.0.0.1', 6002)
                node_sock = socket .socket(socket .AF_INET, socket .SOCK_DGRAM)
                msg = json.dumps({ 'lat': lat, 'lng': lon }).encode()
                node_sock.sendto(msg, NODE_SERVER_ADDR)
            elif ptype == 'done':
                robot_sock.sendto(f"ACK_DONE:{num}".encode(), addr)
                logger.info("ACK_DONE sent")
                if returned:
                    cmd = json.dumps({'type':'cmd','action':'pause'}).encode()
                    robot_sock.sendto(cmd, addr)
                    logger.info("Final pause sent, exiting loop")
                    break
        robot_sock.close()
        logger.info("Robot socket closed, server_loop 종료")

    def send_command(self, action):
        pkt = json.dumps({'type':'cmd','action':action}).encode()
        self.robot_sock.sendto(pkt, (CLIENT_IP, CLIENT_PORT))
        logger.info(f'Control -> Robot: {action}')

if __name__ == '__main__':
    pid_file = os.path.join(BASE_DIR, 'server.pid')
    daemon = DeliveryDaemon(
        pid_file,
        stdin='/dev/null', stdout=log_conf['SERVER_LOG_FILE'], stderr=log_conf['SERVER_LOG_FILE']
    )
    if len(sys.argv) == 2:
        cmd = sys.argv[1]
        if cmd == 'start':
            daemon.start()
        elif cmd == 'stop':
            daemon.stop()
        elif cmd == 'restart':
            daemon.restart()
        elif cmd == 'status':
            daemon.status()
        else:
            print('Usage: server.py [start|stop|restart|status]')
    else:
        print('Usage: server.py [start|stop|restart|status]')
