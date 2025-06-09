import socket, json, time, threading, os, sys, logging
from daemon_base import Daemon

# 설정 파일 로드 (Raspberry Pi 경로)
with open("/home/hyunmin/SixWheelBot/Communication/config/comm_config.json", "r", encoding="utf-8") as f:
    config = json.load(f)
cli_conf = config["CLIENT"]
log_conf = config["LOG"]
# db_conf  = config["DB"]

SERVER_IP   = cli_conf["SERVER_IP"]
SERVER_PORT = int(cli_conf["SERVER_PORT"])
CLIENT_IP   = cli_conf["CLIENT_IP"]
CLIENT_PORT = int(cli_conf["CLIENT_PORT"])
ACK_TIMEOUT = float(cli_conf["ACK_TIMEOUT"])
RETRY_LIMIT = int(cli_conf["RETRY_LIMIT"])
LOG_MODE    = log_conf["LOG_MODE"].upper()
LOG_FILE    = log_conf["CLIENT_LOG_FILE"]

logger = logging.getLogger("ClientDaemon")
logger.setLevel(getattr(logging, LOG_MODE))
fh = logging.FileHandler(LOG_FILE, mode="a+")
fh.setFormatter(logging.Formatter("%(asctime)s [%(levelname)s] %(message)s"))
logger.addHandler(fh)

# 캐시 구조
packet_cache = {}
received_cmds = set()
received_done = set()

# 소켓 바인딩
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
if CLIENT_IP != '0.0.0.0':
    sock.bind((CLIENT_IP, CLIENT_PORT))
sock.settimeout(None)
logger.info(f'Client ready on {CLIENT_IP}:{CLIENT_PORT}')

# 클라이언트 실행: 서버로부터 command 수신 및 처리
def client_loop():
    while True:
        data, addr = sock.recvfrom(65536)
        msg = data.decode(errors='ignore').strip()

        # 재전송 요청 처리
        if msg.startswith('RETRANS_CMD:'):
            req = int(msg.split(':')[1])
            if req in packet_cache:
                sock.sendto(packet_cache[req], (SERVER_IP, SERVER_PORT))
                logger.info(f'Retransmitted packet {req}')
            continue

        if not msg.startswith('{'):
            continue
        pkt = json.loads(msg)
        ptype = pkt.get('type')
        num = pkt.get('packet_number')

        # 중복 명령 ACK
        if ptype == 'command' and num in received_cmds:
            sock.sendto(f'ACK_CMD:{num}'.encode(), addr)
            continue

        if ptype == 'command':
            received_cmds.add(num)
            sock.sendto(f'ACK_CMD:{num}'.encode(), addr)
            logger.info(f'Received command {num} and ACK_CMD sent')
            # execute route
            for i, point in enumerate(pkt.get('route', [])):
                log_num = i
                log_pkt = {
                    'packet_number': log_num,
                    'type': 'log',
                    'robot_state': 'moving',
                    'gps': point,
                    'timestamp': time.time()
                }
                data_log = json.dumps(log_pkt).encode()
                packet_cache[log_num] = data_log
                # send with retry
                for retry in range(RETRY_LIMIT):
                    sock.sendto(data_log, (SERVER_IP, SERVER_PORT))
                    data_ack, _ = sock.recvfrom(1024)
                    if data_ack.decode() == f'ACK_LOG:{log_num}':
                        logger.info(f'ACK_LOG {log_num} received')
                        del packet_cache[log_num]
                        break
                    else:
                        logger.warning(f'ACK_LOG retry {retry+1}')
                time.sleep(1)

            # done packet
            done_num = len(pkt.get('route', []))
            done_pkt = {
                'packet_number': done_num,
                'type': 'done',
                'timestamp': time.time()
            }
            data_done = json.dumps(done_pkt).encode()
            packet_cache[done_num] = data_done
            for retry in range(RETRY_LIMIT):
                sock.sendto(data_done, (SERVER_IP, SERVER_PORT))
                data_ack, _ = sock.recvfrom(1024)
                if data_ack.decode() == f'ACK_DONE:{done_num}':
                    logger.info(f'ACK_DONE {done_num} received')
                    del packet_cache[done_num]
                    break
                else:
                    logger.warning(f'ACK_DONE retry {retry+1}')
            logger.info('Delivery done')

class ClientDaemon(Daemon):
    def run(self):
        client_loop()

if __name__ == '__main__':
    pidf = '/var/run/client.pid'
    daemon = ClientDaemon(pidf, stdout=LOG_FILE, stderr=LOG_FILE)
    if len(sys.argv) == 2:
        cmd = sys.argv[1]
        if cmd == 'start':   daemon.start()
        elif cmd == 'stop':    daemon.stop()
        elif cmd == 'status':  daemon.status()
        elif cmd == 'restart': daemon.restart()
        else: print('Usage: client_daemon.py [start|stop|status|restart]')
    else:
        client_loop()