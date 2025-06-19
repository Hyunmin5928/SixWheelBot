import call_map_api

from call_map_api import fetch_route_data, parse_route_coords
import sys

API_KEY = "6uHPB650j41F9NmAfTKjs5DxEZ0eBcTC77dm55iX"

# ✏️ 여기서 출발지/도착지 좌표 지정
START_LAT = 37.3388
START_LON = 127.1093
END_LAT   = 37.3379
END_LON   = 127.1097

def save_coords_to_txt(route_coords, filepath="./tmp/coords.txt"):
    try:
        with open(filepath, "w", encoding="utf-8") as f:
            for lat, lon, turn_type in route_coords:
                t_str = str(turn_type) if turn_type is not None else "없음"
                f.write(f"{lat},{lon},{t_str}\n")
        print(f"✅ 좌표를 {filepath}에 저장 완료")
        #통신으로 .txt 파일 전송

        #####################
    except Exception as e:
        print(f"❌ 파일 저장 실패: {e}")

def main():
    try:
        print("📡 경로 요청 중...")
        data = fetch_route_data(START_LAT, START_LON, END_LAT, END_LON, API_KEY)

        route_coords, _, _ = parse_route_coords(data)

        print(f"📍 받은 경로 좌표 수: {len(route_coords)}개")
        save_coords_to_txt(route_coords)

    except Exception as e:
        print("❌ 오류 발생:", e)
        sys.exit(1)

if __name__ == "__main__":
    main()
