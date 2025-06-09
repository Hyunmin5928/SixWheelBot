import socket, json, time
import threading, os, sys
from daemon_base import Daemon
from collections import deque

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
LOG_FILE    = log_conf.get("CLIENT_LOG_FILE", "comm_log.txt")

import logging
logger = logging.getLogger("ClientDaemon")
logger.setLevel(getattr(logging, LOG_MODE))
fh = logging.FileHandler(LOG_FILE, mode="a+")
fh.setFormatter(logging.Formatter("%(asctime)s [%(levelname)s] %(message)s"))
logger.addHandler(fh)

# 전역 상태
robot_state = "idle"
route = []
packet_num = 0

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind((CLIENT_IP, CLIENT_PORT))
sock.settimeout(ACK_TIMEOUT)

def recv_loop():
    global robot_state, route
    while True:
        try:
            data, addr = sock.recvfrom(4096)
            msg = data.decode().strip()

            # ACK 메시지는 여기서 처리하지 않고 패스
            if msg.startswith("ACK:"):
                logger.debug(f"recv_loop에서 ACK 무시: {msg}")
                continue
            # JSON 메시지만 파싱
            if not msg.startswith("{"):
                logger.warning(f"수신 알 수 없는 메시지 무시: {msg}")
                continue

            pkt = json.loads(msg)
            if pkt.get("type") == "command":
                route = pkt.get("route", [])
                logger.info(f"경로 수신: {len(route)} 지점")
                robot_state = "moving"

        except socket.timeout:
            logger.debug("명령 대기 중 (timeout)")
        except Exception as e:
            logger.error(f"수신 오류: {e}")

def client_loop():
    global packet_num, robot_state, route
    while True:
        try:
            # R1: 1초마다 상태+GPS 로그 전송
            pkt = {
                "packet_number": packet_num,
                "type": "log",
                "robot_state": robot_state,
                # 이동 중(route이 있으면 그 지점, 아니면 현재값 그대로)
                "gps": route[0] if route else {"lat": 0.0, "lon": 0.0},
                "timestamp": time.time()
            }
            data = json.dumps(pkt).encode()

            # R3,R6,R7: ACK 미수신 시 재전송
            for retry in range(RETRY_LIMIT):
                sock.sendto(data, (SERVER_IP, SERVER_PORT))
                try:
                    ack, _ = sock.recvfrom(1024)
                    if ack.decode() == f"ACK:{packet_num}":
                        logger.info(f"ACK-{packet_num} 수신")
                        break
                except socket.timeout:
                    logger.warning(f"ACK 재시도 {retry+1}/{RETRY_LIMIT}")
            packet_num += 1

            # 이동 경로 수행 후 완료 메시지
            if robot_state == "moving" and route:
                for pt in route[1:]:
                    # 이미 첫 지점은 위에서 보냈으므로 순회
                    pkt = {
                        "packet_number": packet_num,
                        "type": "log",
                        "robot_state": robot_state,
                        "gps": pt,
                        "timestamp": time.time()
                    }
                    data = json.dumps(pkt).encode()
                    for retry in range(RETRY_LIMIT):
                        sock.sendto(data, (SERVER_IP, SERVER_PORT))
                        try:
                            ack, _ = sock.recvfrom(1024)
                            if ack.decode() == f"ACK:{packet_num}":
                                logger.info(f"ACK-{packet_num} 수신")
                                break
                        except socket.timeout:
                            logger.warning(f"ACK 재시도 {retry+1}/{RETRY_LIMIT}")
                    packet_num += 1
                    time.sleep(1)

                # 완료 메시지 전송
                done = {
                    "packet_number": packet_num,
                    "type": "done",
                    "timestamp": time.time()
                }
                sock.sendto(json.dumps(done).encode(), (SERVER_IP, SERVER_PORT))
                logger.info("배달 완료 메시지 전송")
                robot_state = "idle"
                packet_num += 1

        except Exception as e:
            logger.error(f"송신 오류: {e}")
        time.sleep(1)

class ClientDaemon(Daemon):
    def run(self):
        threading.Thread(target=recv_loop, daemon=True).start()
        client_loop()

if __name__ == "__main__":
    pidf = "/var/run/client.pid"
    daemon = ClientDaemon(pidf, stdout=LOG_FILE, stderr=LOG_FILE)
    if len(sys.argv) == 2:
        cmd = sys.argv[1]
        if cmd == "start":   daemon.start()
        elif cmd == "stop":    daemon.stop()
        elif cmd == "status":  daemon.status()
        elif cmd == "restart": daemon.restart()
        else: print("Usage: client.py [start|stop|status|restart]")
    else:
        # Raspberry Pi에서 직접 테스트 모드로 실행하려면 아래 주석 해제
        # print("== Foreground 모드로 클라이언트 실행 ==")
        # recv_loop(); client_loop()
        print("Usage: client.py [start|stop|status|restart]")
