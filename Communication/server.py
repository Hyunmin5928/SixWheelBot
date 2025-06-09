import socket, json, time, sqlite3
import threading, os, sys
from daemon_base import Daemon
from collections import deque

# 설정 파일 로드 (Windows 테스트 시 사용)
# with open("/home/hyunmin/SixWheelBot/Communication/config/comm_config.json", "r", encoding="utf-8") as f:
with open("C:\\vscode\\SixWheelBot\\Communication\\config\\config.json", "r", encoding="utf-8") as f:
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
LOG_FILE    = log_conf.get("SERVER_LOG_FILE", "log\\comm_log.txt")
DB_FILE     = db_conf.get("ROBOT_DB_FILE", "log\\robot_log.db")

import logging
logger = logging.getLogger("ServerDaemon")
logger.setLevel(getattr(logging, LOG_MODE))
fh = logging.FileHandler(LOG_FILE, mode="a+", encoding="utf-8")
fh.setFormatter(logging.Formatter("%(asctime)s [%(levelname)s] %(message)s"))
logger.addHandler(fh)

def save_to_db(data):
    conn = sqlite3.connect(DB_FILE)
    cur = conn.cursor()
    cur.execute("""
CREATE TABLE IF NOT EXISTS robot_logs (
    id INTEGER PRIMARY KEY,
    timestamp REAL, state TEXT, lat REAL, lon REAL
)""")
    cur.execute(
        "INSERT INTO robot_logs (timestamp,state,lat,lon) VALUES (?,?,?,?)",
        (data.get("timestamp"),
         data.get("robot_state"),
         data.get("gps", {}).get("lat"),
         data.get("gps", {}).get("lon"))
    )
    conn.commit()
    conn.close()

def server_loop():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((SERVER_IP, SERVER_PORT))
    logger.info(f"서버 바인드 완료: {SERVER_IP}:{SERVER_PORT}")
    sock.settimeout(ACK_TIMEOUT)

    # 최초 클라이언트로 초기 경로 전송
    try:
        initial_route = [
            {"lat": 37.123, "lon": 127.123},
            {"lat": 37.124, "lon": 127.124},
            {"lat": 37.125, "lon": 127.125},
        ]
        init_cmd = {
            "packet_number": 0,
            "type": "command",
            "route": initial_route,
            "timestamp": time.time()
        }
        sock.sendto(json.dumps(init_cmd).encode(), (CLIENT_IP, CLIENT_PORT))
        logger.info("초기 경로 패킷 전송 완료")
    except Exception as e:
        logger.error(f"초기 경로 전송 실패: {e}")

    send_flag = True
    received = set()
    expected = 0

    while True:
        try:
            data, addr = sock.recvfrom(4096)
            msg = data.decode().strip()

            # JSON 메시지가 아니면 무시
            if not msg.startswith("{"):
                logger.debug(f"서버 수신 비JSON 메시지 무시: {msg}")
                continue

            pkt = json.loads(msg)
            num = pkt.get("packet_number", -1)

            if num in received:
                logger.debug(f"중복 패킷 {num}")
                continue
            received.add(num)

            if num != expected:
                logger.warning(f"순서 누락: 기대={expected}, 실제={num}")
            expected = num + 1

            t = pkt.get("type")
            if t == "log":
                logger.info(f"로그 수신: {pkt}")
                save_to_db(pkt)
                # ACK 응답
                sock.sendto(f"ACK:{num}".encode(), addr)
            elif t == "done":
                logger.info("배달 완료 수신")
                sock.sendto(f"ACK:{num}".encode(), addr)
                send_flag = True

            if send_flag:
                route = [
                    {"lat": 37.123, "lon": 127.123},
                    {"lat": 37.124, "lon": 127.124},
                    {"lat": 37.125, "lon": 127.125},
                ]
                cmd = {
                    "packet_number": num + 1000,
                    "type": "command",
                    "route": route,
                    "timestamp": time.time()
                }
                sock.sendto(json.dumps(cmd).encode(), addr)
                logger.info("경로 전송 완료")
                send_flag = False

        except socket.timeout:
            logger.debug("수신 대기 중 (timeout)")
        except Exception as e:
            logger.error(f"서버 오류: {e}")

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
