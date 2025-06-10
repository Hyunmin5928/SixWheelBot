import os, sys, socket, json, time, logging
from daemon_base import Daemon

# 설정 파일 로드
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
CONFIG_PATH = os.path.join(BASE_DIR, 'config', 'config.json')
with open(CONFIG_PATH, 'r', encoding='utf-8') as f:
    config = json.load(f)

# 설정 파싱
srv_conf = config["SERVER"]
cli_conf = config["CLIENT"]
net_conf = config["NETWORK"]
log_conf = config["LOG"]

SERVER_IP   = srv_conf["IP"]
SERVER_PORT = int(srv_conf["PORT"])
CLIENT_IP   = cli_conf["IP"]
CLIENT_PORT = int(cli_conf["PORT"])
ALLOW_IP    = net_conf["ALLOW_IP"]
ACK_TIMEOUT = float(net_conf["ACK_TIMEOUT"])
RETRY_LIMIT = int(net_conf["RETRY_LIMIT"])
LOG_MODE    = log_conf["LOG_MODE"].upper()
LOG_FILE    = log_conf["CLIENT_LOG_FILE"]

# 로거 설정
logger = logging.getLogger("ClientDaemon")
logger.setLevel(getattr(logging, LOG_MODE, logging.INFO))
fh = logging.FileHandler(LOG_FILE, mode='a+', encoding='utf-8')
fh.setFormatter(logging.Formatter("%(asctime)s [%(levelname)s] %(message)s"))
logger.addHandler(fh)

class ClientDaemon(Daemon):
    def run(self):
        self.client_loop()

    def client_loop(self):
        # 소켓 생성 및 바인딩은 start 시에만 실행
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        bind_ip = '' if ALLOW_IP == '0.0.0.0' else ALLOW_IP
        # bind_ip = '' if CLIENT_IP == '0.0.0.0' else CLIENT_IP
        sock.bind((bind_ip, CLIENT_PORT))
        sock.settimeout(ACK_TIMEOUT)
        logger.info(f'Client listening on {bind_ip or "0.0.0.0"}:{CLIENT_PORT}')

        packet_cache = {}
        received_map = False

        while True:
            try:
                data, addr = sock.recvfrom(65536)
            except socket.timeout:
                continue

            msg = data.decode(errors='ignore').strip()

            # 재전송 요청 처리
            if msg.startswith('RETRANS_MAP:'):
                sock.sendto(packet_cache.get(0, b''), addr)
                logger.info('Retransmitted map')
                continue
            if msg.startswith('RETRANS_LOG:'):
                num = int(msg.split(':')[1])
                sock.sendto(packet_cache.get(num, b''), (SERVER_IP, SERVER_PORT))
                logger.info(f'Retransmitted log {num}')
                continue
            if msg.startswith('RETRANS_DONE:'):
                num = int(msg.split(':')[1])
                sock.sendto(packet_cache.get(num, b''), (SERVER_IP, SERVER_PORT))
                logger.info(f'Retransmitted done {num}')
                continue

            # JSON 패킷만 파싱
            if not msg.startswith('{'):
                continue

            pkt = json.loads(msg)
            ptype = pkt.get('type')
            num = pkt.get('packet_number')

            # map 처리
            if ptype == 'map' and not received_map:
                packet_cache[0] = data
                sock.sendto(b'ACK_MAP:0', addr)
                route = pkt.get('route', [])
                logger.info(f'Map received: {len(route)} points')
                received_map = True

                # 경로 로그 전송
                for i, pt in enumerate(route):
                    log_pkt = {
                        'packet_number': i,
                        'type': 'log',
                        'robot_state': 'moving',
                        'gps': pt,
                        'timestamp': time.time()
                    }
                    data_log = json.dumps(log_pkt).encode()
                    packet_cache[i] = data_log
                    # ACK_LOG 대기 및 재전송
                    for attempt in range(RETRY_LIMIT):
                        sock.sendto(data_log, (SERVER_IP, SERVER_PORT))
                        try:
                            ack, _ = sock.recvfrom(1024)
                            if ack.decode() == f'ACK_LOG:{i}':
                                del packet_cache[i]
                                break
                        except socket.timeout:
                            logger.warning(f'ACK_LOG retry {attempt+1} for {i}')
                            continue
                    time.sleep(2)

                # done 신호 전송
                done_num = len(route)
                done_pkt = {
                    'packet_number': done_num,
                    'type': 'done',
                    'timestamp': time.time()
                }
                data_done = json.dumps(done_pkt).encode()
                packet_cache[done_num] = data_done
                for attempt in range(RETRY_LIMIT):
                    sock.sendto(data_done, (SERVER_IP, SERVER_PORT))
                    try:
                        ack, _ = sock.recvfrom(1024)
                        if ack.decode() == f'ACK_DONE:{done_num}':
                            del packet_cache[done_num]
                            break
                    except socket.timeout:
                        logger.warning(f'ACK_DONE retry {attempt+1} for done')
                        continue
                logger.info('Delivery completed')
                break

if __name__ == '__main__':
    pidf = '/var/run/client.pid'
    daemon = ClientDaemon(pidf, stdout=LOG_FILE, stderr=LOG_FILE)
    if len(sys.argv) == 2:
        cmd = sys.argv[1].lower()
        if cmd == 'start':
            logger.info(f'Client Daemon {cmd}')
            daemon.start()
        elif cmd == 'stop':
            logger.info(f'Client Daemon {cmd}')
            daemon.stop()
        elif cmd == 'status':
            daemon.status()
        elif cmd == 'restart':
            logger.info(f'Client Daemon {cmd}')
            daemon.restart()
        else:
            print('Usage: client.py [start|stop|status|restart]')
    else:
        # 직접 실행 시에도 루프 실행
        ClientDaemon(pidf, stdout=LOG_FILE, stderr=LOG_FILE).run()