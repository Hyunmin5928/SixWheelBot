import requests
import json

# 🔧 설정
API_KEY = "6uHPB650j41F9NmAfTKjs5DxEZ0eBcTC77dm55iX"  
START_LAT = 37.33885769999998 
START_LON = 127.10922419999957
END_LAT   = 37.33789480000011
END_LON   = 127.10969899999932

TURN_TYPE_MAP = {
    10: "직진",
    11: "좌회전",
    12: "좌회전",
    13: "우회전",
    14: "유턴",
    16: "1시 방향 우회전",
    17: "1시 방향 우회전",
    18: "2시 방향 우회전",
    19: "10시 방향 좌회전",
    20: "11시 방향 좌회전",
    211: "횡단보도 건너기",
    200: "출발지",
    201: "도착지"
}



# 1. Tmap 도보 길찾기 API 호출
def fetch_route_data(start_lat, start_lon, end_lat, end_lon, api_key):
    url = "https://apis.openapi.sk.com/tmap/routes/pedestrian?version=1"
    headers = {
        "appKey": api_key,
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

    response = requests.post(url, headers=headers, json=body)
    if response.status_code == 200:
        return response.json()
    else:
        raise Exception(f"API 호출 실패: {response.status_code} / {response.text}")


def parse_route_coords(data):
    route_coords = []
    coord_map = {}  # (lat, lon) → turnType (None or int)
    crosswalk_coords = []
    turn_points = []

    for feature in data["features"]:
        geometry = feature["geometry"]
        props = feature["properties"]
        turn_type = props.get("turnType")
        facility_type = props.get("facilityType")

        if geometry["type"] == "LineString":
            for lon, lat in geometry["coordinates"]:
                coord = (lat, lon)
                if coord not in coord_map:
                    coord_map[coord] = None  # 기본값

        elif geometry["type"] == "Point":
            lat, lon = geometry["coordinates"][1], geometry["coordinates"][0]
            coord = (lat, lon)

            # 📌 Point가 가진 turnType이 있으면 우선 등록
            coord_map[coord] = turn_type

            if turn_type == 211 and facility_type == "15":
                crosswalk_coords.append(coord)

            if turn_type in TURN_TYPE_MAP:
                turn_points.append({
                    "coord": coord,
                    "turnType": turn_type,
                    "direction": TURN_TYPE_MAP[turn_type]
                })

    # ✅ 좌표 리스트 구성 (turnType 포함)
    route_coords = [(lat, lon, coord_map[(lat, lon)]) for (lat, lon) in coord_map]

    return route_coords, crosswalk_coords, turn_points





#  2. 좌표 출력 함수
def print_coords(label, coords):
    print(f"\n📌 {label}")
    for lat, lon in coords:
        print(f"{lat:.7f}, {lon:.7f}")
        
def print_turn_points(turn_points):
    print("\n🔁 회전 안내 지점:")
    for tp in turn_points:
        lat, lon = tp["coord"]
        print(f"{lat:.7f}, {lon:.7f} → {tp['direction']} (turnType={tp['turnType']})")

def print_route_with_turntypes(route_coords):
    print("\n📍 전체 좌표 + turnType:")
    for lat, lon, turn_type in route_coords:
        t_str = str(turn_type) if turn_type is not None else "없음"
        print(f"{lat:.7f}, {lon:.7f}, {t_str}")
       



def main():
    try:
        data = fetch_route_data(START_LAT, START_LON, END_LAT, END_LON, API_KEY)

        route_coords, crosswalk_coords, turn_points = parse_route_coords(data)

        print_route_with_turntypes(route_coords)
       # print_coords("전체 경로 좌표", route_coords)
        print_coords("횡단보도 좌표", crosswalk_coords)
        print_turn_points(turn_points)

    except Exception as e:
        print("❌ 오류 발생:", e)



#  프로그램 실행
if __name__ == "__main__":
    main()
