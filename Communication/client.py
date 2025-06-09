import socket, json, time, threading

with open("comm_config.json", "r") as f:
    config = json.load(f)

SERVER_IP = config["SERVER_IP"]
SERVER_PORT = int(config["SERVER_PORT"])
CLIENT_IP = config["CLIENT_IP"]
CLIENT_PORT = int(config["CLIENT_PORT"])
ACK_TIMEOUT = float(config["ACK_TIMEOUT"])
RETRY_LIMIT = int(config["RETRY_LIMIT"])


def log(level, msg):
    if level.lower() in ["info", "warn", "debug", "error"]:
        print(f"[{level.upper()}] {msg}")

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((CLIENT_IP, CLIENT_PORT))
sock.settimeout(ACK_TIMEOUT)
packet_num = 0
packet_cache = {}

robot_state = "idle"
destination = None

def receive_loop():
    global robot_state, destination
    while True:
        try:
            data, addr = sock.recvfrom(4096)
            data_dict = json.loads(data.decode())
            recv_num = data_dict.get("packet_number", -1)
            log("info", f"서버 명령 수신: {data_dict}")
            sock.sendto(f"ACK:{recv_num}".encode(), addr)

            if "destination" in data_dict:
                destination = data_dict["destination"]
                log("info", f"[목적지 명령] 이동 좌표: {destination}")
                robot_state = "moving"

        except Exception as e:
            log("error", f"클라이언트 수신 오류: {e}")


recv_thread = threading.Thread(target=receive_loop, daemon=True)
recv_thread.start()

while True:
    try:
        packet = {
            "packet_number": packet_num,
            "robot_state": robot_state,
            "gps": {"lat": 37.123, "lon": 127.123},
            "timestamp": time.time()
        }
        encoded = json.dumps(packet).encode()
        packet_cache[packet_num] = encoded
        retry = 0

        while retry < RETRY_LIMIT:
            sock.sendto(encoded, (SERVER_IP, SERVER_PORT))
            try:
                ack, _ = sock.recvfrom(1024)
                if ack.decode() == f"ACK:{packet_num}":
                    log("info", f"ACK 수신 성공 (패킷 {packet_num})")
                    break
            except socket.timeout:
                retry += 1
                log("warn", f"ACK 재시도 {retry}/{RETRY_LIMIT}")

        packet_num += 1
        time.sleep(1)

    except Exception as e:
        log("error", f"클라이언트 송신 오류: {e}")