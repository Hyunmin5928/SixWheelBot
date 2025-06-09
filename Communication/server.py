import socket, json, time, sqlite3

with open("config/comm_config.json", "r") as f:
    config = json.load(f)

SERVER_IP = config["SERVER_IP"]
SERVER_PORT = int(config["SERVER_PORT"])
ACK_TIMEOUT = float(config["ACK_TIMEOUT"])
LOG_MODE = config["LOG_MODE"]


def log(level, msg):
    if level.lower() in ["info", "warn", "debug", "error"]:
        print(f"[{level.upper()}] {msg}")

def save_to_db(data):
    conn = sqlite3.connect("robot_log.db")
    cur = conn.cursor()
    cur.execute("""
        CREATE TABLE IF NOT EXISTS robot_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp REAL,
            state TEXT,
            lat REAL,
            lon REAL
        )
    """)
    cur.execute("""
        INSERT INTO robot_logs (timestamp, state, lat, lon)
        VALUES (?, ?, ?, ?)
    """, (data["timestamp"], data["robot_state"], data["gps"]["lat"], data["gps"]["lon"]))
    conn.commit()
    conn.close()

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((SERVER_IP, SERVER_PORT))
sock.settimeout(ACK_TIMEOUT)

while True:
    try:
        data, addr = sock.recvfrom(4096)
        data_dict = json.loads(data.decode())
        packet_num = data_dict.get("packet_number", -1)

        log("info", f"로봇 데이터 수신: {data_dict}")
        # save_to_db(data_dict)
        sock.sendto(f"ACK:{packet_num}".encode(), addr)

        # 상태 idle일 때 목적지 전송 예시
        if data_dict.get("robot_state") == "idle":
            destination_packet = {
                "packet_number": packet_num + 1000,
                "destination": {"lat": 37.555, "lon": 127.888},
                "timestamp": time.time()
            }
            sock.sendto(json.dumps(destination_packet).encode(), addr)

    except Exception as e:
        log("error", f"서버 수신 오류: {e}")
