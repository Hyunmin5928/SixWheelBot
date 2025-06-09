#!/usr/bin/env python3
import serial
import time
import re
import requests

# ───────────────────────────────────────────────────────────
# 1) 시리얼 포트 설정
#   • USB 케이블로 연결된 Arduino라면 '/dev/ttyACM0' 또는 '/dev/ttyUSB0'
#   • GPIO UART 연결 시 '/dev/serial0'
#   • Baudrate는 Arduino 쪽 Serial.begin(9600)과 동일하게 설정
# ───────────────────────────────────────────────────────────
SERIAL_PORT = '/dev/ttyUSB0'  # 필요에 따라 '/dev/ttyACM0' 또는 '/dev/serial0' 등으로 변경
BAUDRATE    = 9600
TIMEOUT_SEC = 1  # readline 시 타임아웃 (초)

# ───────────────────────────────────────────────────────────
# 2) 정규표현식 정의
#    • Arduino에서 "LAT:37.340091, LNG:127.108947" 형식으로 보낼 때를 매치
#    • 그룹 1 = 위도, 그룹 2 = 경도
# ───────────────────────────────────────────────────────────
pattern = re.compile(r'^\s*LAT: *(-?\d+\.\d+)\s*,\s*LNG: *(-?\d+\.\d+)\s*$')

def main():
    # 2-1) Serial 포트 열기
    try:
        ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=TIMEOUT_SEC)
    except serial.SerialException as e:
        print(f"❌ 시리얼 포트를 열 수 없음: {e}")
        return

    print(f"📡 Serial opened on {SERIAL_PORT} at {BAUDRATE}bps")
    time.sleep(2)  # Arduino 리셋/시작 시 초기 garbage 데이터 버리기

    # 2-2) Arduino로부터 GPS 좌표 수신
    print("📡 아두이노 GPS 좌표 수신 대기 중...")
    start_lat = None
    start_lon = None

    try:
        while True:
            raw_line = ser.readline().decode('utf-8', errors='ignore').strip()
            if not raw_line:
                # 빈 줄 또는 timeout
                continue

            # 데이터 예시:
            #  1) "LAT:37.340091, LNG:127.108947"
            #  2) 혹은 "37.340091,127.108947" (만약 Arduino에서 쉼표만 보낼 경우)
            print(f"Received raw: {raw_line}")

            # 정규표현식 매칭 시도 ( "LAT:..., LNG:..." )
            m = pattern.match(raw_line)
            if m:
                start_lat = float(m.group(1))
                start_lon = float(m.group(2))
                print(f"✅ 파싱된 GPS 좌표 (PREFIX) → 위도: {start_lat:.6f}, 경도: {start_lon:.6f}")
                break

            # 만약 Arduino에서 "위도,경도" 형식으로만 보낼 경우
            # ex) "37.340091,127.108947"
            try:
                # 쉼표 기준으로 분리
                lat_str, lon_str = raw_line.split(',')
                start_lat = float(lat_str)
                start_lon = float(lon_str)
                print(f"✅ 파싱된 GPS 좌표 (RAW)   → 위도: {start_lat:.6f}, 경도: {start_lon:.6f}")
                break
            except Exception:
                # 매칭되지 않는다면 무시하고 다시 읽기
                continue

    except KeyboardInterrupt:
        print("\n🔴 사용자가 중단함")
        ser.close()
        return

    # 2-3) Serial 포트 닫기
    ser.close()
    print("[*] Serial closed\n")

    # ───────────────────────────────────────────────────────────
    # 3) 도착 좌표 (임의 설정)
    #    • 예시: 대전↔대전역 등 원하는 위경도로 교체 가능
    # ───────────────────────────────────────────────────────────
    end_lat, end_lon = 37.33838, 127.10900
    print(f"📌 도착 좌표: 위도 {end_lat}, 경도 {end_lon}")

    # ───────────────────────────────────────────────────────────
    # 4) TMAP 도보 길찾기 API 호출
    #    • https://apis.openapi.sk.com/tmap/routes/pedestrian?version=1
    #    • APP KEY는 본인 발급 키로 교체
    # ───────────────────────────────────────────────────────────
    url = "https://apis.openapi.sk.com/tmap/routes/pedestrian?version=1"
    headers = {
        "appKey": "6uHPB650j41F9NmAfTKjs5DxEZ0eBcTC77dm55iX",  # 본인 키로 교체
        "Content-Type": "application/json"
    }
    body = {
        "startX": str(start_lon),
        "startY": str(start_lat),
        "endX": str(end_lon),
        "endY": str(end_lat),
        "reqCoordType": "WGS84GEO",
        "resCoordType": "WGS84GEO",
        "startName": "출발지",
        "endName": "도착지"
    }

    print("\n🚀 TMAP API 호출 중...")
    response = requests.post(url, headers=headers, json=body)

    if response.status_code == 200:
        data = response.json()
        route_coords = []

        # TMAP 응답의 features 안에서 geometry.type=="LineString" 데이터를 추출
        for feature in data.get("features", []):
            geometry = feature.get("geometry", {})
            if geometry.get("type") == "LineString":
                # coordinates: [ [lon, lat], [lon, lat], ... ]
                for lon, lat in geometry.get("coordinates", []):
                    route_coords.append((lat, lon))

        if route_coords:
            print("\n🗺️ Total road location (경로 좌표):")
            for i, (lat, lon) in enumerate(route_coords, start=1):
                print(f"{i:>3}. 위도: {lat:.7f}, 경도: {lon:.7f}")
        else:
            print("⚠️ 경로 정보(LineString)를 찾을 수 없습니다.")
    else:
        print("❌ TMAP API 호출 실패")
        print(f"   STATUS CODE: {response.status_code}")
        print(f"   RESPONSE TEXT: {response.text}")


if __name__ == "__main__":
    main()

