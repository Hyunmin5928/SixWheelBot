import socket, json, time, os, sys, logging
from daemon_base import Daemon

# 설정 파일 로드 (Raspberry Pi 경로)
# with open("/home/hyunmin/SixWheelBot/Communication/config/comm_config.json", "r", encoding="utf-8") as f:
#     config = json.load(f)
# 설정 로드
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
CONFIG_PATH = os.path.join(BASE_DIR,'config','comm_config.json')
with open(CONFIG_PATH,'r',encoding='utf-8') as f:
    config=json.load(f)

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

# 바인딩
sock=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
if CLIENT_IP!='0.0.0.0': sock.bind((CLIENT_IP,CLIENT_PORT))
sock.settimeout(ACK_TIMEOUT)
logger.info(f'Client listening on {CLIENT_IP}:{CLIENT_PORT}')

# 클라이언트 실행: 서버로부터 command 수신 및 처리
def client_loop():
    packet_cache = {}
    received_map = False

    while True:
        try:
            data,addr = sock.recvfrom(65536)
        except socket.timeout:
            continue
        msg = data.decode(errors='ignore').strip()

        # 재전송 요청 처리
        if msg.startswith('RETRANS_MAP:'):
            sock.sendto(packet_cache.get(0, b''), addr)
            logger.info('Retransmitted map')
            continue
        if msg.startswith('RETRANS_LOG:'):
            num=int(msg.split(':')[1])
            sock.sendto(packet_cache.get(num,b''),(SERVER_IP,SERVER_PORT))
            logger.info(f'Retransmitted log {num}')
            continue
        if msg.startswith('RETRANS_DONE:'):
            num=int(msg.split(':')[1])
            sock.sendto(packet_cache.get(num,b''),(SERVER_IP,SERVER_PORT))
            logger.info(f'Retransmitted done {num}')
            continue

        if not msg.startswith('{'):
            continue
        pkt = json.loads(msg)
        ptype = pkt.get('type')
        num = pkt.get('packet_number')

        if ptype=='map' and not received_map:
            packet_cache[0] = data
            sock.sendto(b'ACK_MAP:0', addr)
            route=pkt.get('route', [])
            logger.info(f'Map received: {len(route)} points')
            received_map=True

            # send logs
            for i,pt in enumerate(route):
                log_pkt={'packet_number':i,'type':'log','robot_state':'moving','gps':pt,'timestamp':time.time()}
                data_log=json.dumps(log_pkt).encode()
                packet_cache[i]=data_log
                # retry
                for attempt in range(RETRY_LIMIT):
                    sock.sendto(data_log,(SERVER_IP,SERVER_PORT))
                    try:
                        ack,_=sock.recvfrom(1024)
                        if ack.decode()==f'ACK_LOG:{i}':
                            del packet_cache[i]
                            break
                    except socket.timeout:
                        continue
                time.sleep(2)
                
            # done packet
            done_num=len(route)
            done_pkt={'packet_number':done_num,'type':'done','timestamp':time.time()}
            data_done=json.dumps(done_pkt).encode()
            packet_cache[done_num]=data_done
            for attempt in range(RETRY_LIMIT):
                sock.sendto(data_done,(SERVER_IP,SERVER_PORT))
                try:
                    ack,_=sock.recvfrom(1024)
                    if ack.decode()==f'ACK_DONE:{done_num}':
                        del packet_cache[done_num]
                        break
                except socket.timeout:
                    continue
            logger.info('Delivery completed')
            break

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