#!/usr/bin/env python3
import serial
import time

# 1) 포트와 Baud Rate 설정
SERIAL_PORT = '/dev/ttyUSB0'   # 라즈베리파이에서 확인된 포트
BAUD_RATE   = 115200           # 아두이노 Serial.begin(...) 값과 일치시킬 것

def main():
    # 2) 시리얼 포트 열기
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)  # 리셋 안정화 대기
        print(f"시리얼 포트 열림: {SERIAL_PORT} @ {BAUD_RATE}bps")
    except Exception as e:
        print("시리얼 포트 열기 실패:", e)
        return

    # 3) 무한 루프: 들어오는 데이터 그대로 출력
    try:
        while True:
            line_bytes = ser.readline()          # 한 줄 읽기
            if not line_bytes:
                continue                         # 빈 줄이면 재시도
            # 디코딩·개행 제거
            line_str = line_bytes.decode('utf-8', errors='ignore').strip()
            print(line_str)                      # 터미널에 출력
    except KeyboardInterrupt:
        print("\n프로그램 종료 요청 (Ctrl+C)")
    finally:
        ser.close()
        print("시리얼 포트 닫음")

if __name__ == "__main__":
    main()
