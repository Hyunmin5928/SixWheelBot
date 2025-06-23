import socket, json, time, threading, logging, os, sys

# ── 설정 로드 ────────────────────────────────────────────────────────────────
BASE_DIR    = os.path.dirname(os.path.abspath(__file__))
CONFIG_PATH = os.path.join(BASE_DIR, 'config', 'config.json')
with open(CONFIG_PATH, 'r', encoding='utf-8') as f:
    config = json.load(f)

srv_conf   = config["SERVER"]
cli_conf   = config["CLIENT"]
net_conf   = config["NETWORK"]
log_conf   = config["LOG"]
db_conf    = config["DB"]
MAP_FILE   = config["MAP"]["MAP_FILE"]

SERVER_IP    = srv_conf["IP"]
SERVER_PORT  = int(srv_conf["PORT"])
CLIENT_IP    = cli_conf["IP"]
CLIENT_PORT  = int(cli_conf["PORT"])
CONTROL_PORT = 6001               # Node.js 제어 메시지 수신용 포트
ACK_TIMEOUT  = float(net_conf["ACK_TIMEOUT"])
RETRY_LIMIT  = int(net_conf["RETRY_LIMIT"])

# ── 로거 설정 ────────────────────────────────────────────────────────────────
logger = logging.getLogger("ServerDaemon")
logger.setLevel(getattr(logging, log_conf["LOG_MODE"].upper()))
fh = logging.FileHandler(log_conf["SERVER_LOG_FILE"], mode="a+", encoding="utf-8")
fh.setFormatter(logging.Formatter("%(asctime)s [%(levelname)s] %(message)s"))
logger.addHandler(fh)

# ── UDP 소켓 생성 ──────────────────────────────────────────────────────────
robot_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
robot_sock.bind((SERVER_IP, SERVER_PORT))
robot_sock.settimeout(ACK_TIMEOUT)

control_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
control_sock.bind((SERVER_IP, CONTROL_PORT))


# ── 맵 데이터 로드 함수 ───────────────────────────────────────────────────────
def load_map_data(path):
    route = []
    with open(path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'): continue
            lat, lon, dir_str = line.split(',', 2)
            try:
                route.append({'lat': float(lat),
                              'lon': float(lon),
                              'dir': int(dir_str)})
            except ValueError:
                continue
    return route


# ── 실제 map 전송·로그 수신·명령 전송 루프 ───────────────────────────────────────
def server_loop():
    route = load_map_data(MAP_FILE)
    map_pkt = {
        'packet_number': 0,
        'type': 'map',
        'route': route,
        'timestamp': time.time()
    }
    data_map = json.dumps(map_pkt).encode()

    # 1) Map 전송 & ACK_MAP
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

    # 2) 로그 수신 & 목적지 도착 시 자동 pause/unlock/return
    received = set()
    destination = (route[-1]['lat'], route[-1]['lon'])
    returned = False

    while True:
        try:
            data, addr = robot_sock.recvfrom(65536)
        except socket.timeout:
            continue

        msg = data.decode(errors='ignore').strip()
        if not msg.startswith('{'):
            continue

        pkt = json.loads(msg)
        ptype = pkt.get('type')
        num   = pkt.get('packet_number')

        if ptype == 'log':
            # ACK_LOG 처리
            if num not in received:
                # save_to_db(pkt)  # 필요 시 DB 저장
                received.add(num)
                logger.info(f"Log {num} processed")
            robot_sock.sendto(f"ACK_LOG:{num}".encode(), addr)

            # 위치 확인
            lat, lon = pkt['gps']['lat'], pkt['gps']['lon']
            dist = ((lat-destination[0])**2 + (lon-destination[1])**2)**0.5 * 111000
            if not returned and dist <= 0.5:
                # 자동 명령
                for action in ('pause','unlock','return'):
                    cmd = json.dumps({'type':'cmd','action':action}).encode()
                    robot_sock.sendto(cmd, addr)
                    logger.info(f"{action} command sent (auto)")
                    time.sleep(1 if action!='unlock' else 2)
                returned = True

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


# ── Python 서버에게 보내진 “unlock” / “return” 제어 메시지를 그대로 로봇에 전송 ──
def send_command(action):
    pkt = json.dumps({'type':'cmd','action':action}).encode()
    robot_sock.sendto(pkt, (CLIENT_IP, CLIENT_PORT))
    logger.info(f'Control -> Robot: {action}')


# ── 제어 메시지 수신 리스너 ───────────────────────────────────────────────────
def control_listener():
    logger.info(f"Control listener started on {SERVER_IP}:{CONTROL_PORT}")
    while True:
        data, addr = control_sock.recvfrom(1024)
        msg = json.loads(data.decode())
        t   = msg.get('type')

        if t == 'start':
            logger.info("Start command received, launching server_loop thread")
            threading.Thread(target=server_loop, daemon=True).start()
        elif t in ('unlock','return','pause'):
            logger.info(f"Control command received: {t}")
            send_command(t)
        # 필요시 order_id 활용 로직 추가 가능


if __name__ == '__main__':
    # 제어 리스너 스레드 기동
    threading.Thread(target=control_listener, daemon=True).start()

    # 메인 스레드는 idle 상태로 대기
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        logger.info("Server terminated by user")
