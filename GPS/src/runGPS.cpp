
#include <Python.h>
#include <iostream>
#include "GPS_parse.h"
#include <unistd.h>
#include "logger.h"
#include <vector>
#include <tuple>
#include "IMU_parse.h"
#include "cal_distance.h"
#include <fstream>
#include <sstream>
#include <string>

std::vector<std::tuple<double, double, int>> parsed_coords;
bool finish = false;

void load_coords_from_txt(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "❌ 파일 열기 실패: " << filepath << "\n";
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string lat_str, lon_str, turn_str;

        if (std::getline(iss, lat_str, ',') &&
            std::getline(iss, lon_str, ',') &&
            std::getline(iss, turn_str, ',')) {

            double lat = std::stod(lat_str);
            double lon = std::stod(lon_str);
            int turn = (turn_str == "없음") ? -1 : std::stoi(turn_str);

            parsed_coords.emplace_back(lat, lon, turn);
        }
    }

    std::cout << "✅ .txt 좌표 불러오기 완료: " << parsed_coords.size() << "개\n";
}

void runGPS() {
    std::string cmd = "python3 recv_socket.py";  // 좌표 받아서 /tmp/coords.txt 저장
    system(cmd.c_str());

    std::string filepath = "/tmp/coords.txt";    // 파일 경로 정의
    load_coords_from_txt(filepath);

    GPS gps_device;
    sGPS gps;
    init_logger("gps.log");

    while (true) {
        if(finish){
            break;
        }
        bool G_flag = gps_device.GetGPSdata(&gps);

        if (G_flag) {
            std::cout << "[✅] GPS 데이터 수신 성공!" << std::endl;

            if (!parsed_coords.empty()) {
                auto [target_lat, target_lon, turn] = parsed_coords.front();

                double angle = GeoUtils::bearing(
                    gps.latitude, gps.longitude, target_lat, target_lon);
                double dist = GeoUtils::haversine_distance(gps.latitude, gps.longitude, target_lat, target_lon);
                if(dist<=1){
                    parsed_coords.pop_front();  
                    //도달한 좌표가 횡단보도 or 회전포인트인지 확인 
                    if(turn>0){
                        switch (turn) {
                            case 12: // 좌회전
                            case 212: //좌회전 + 횡단보도
                            
                            case 16: // 8시 방향 좌회전
                            case 214: //8시 방향 + 횡단보도

                            case 17: // 10시 방향 좌회전
                            case 215: //10시 방향 + 횡단보도
                                
                                break;

                            case 13: // 우회전
                            case 213: //우회전 + 횡단보도 

                            case 18: // 2시 방향 우회전
                            case 216: //2시 방향 + 횡단보도

                            case 19: // 4시 방향 우회전
                            case 217: //4시 방향 + 횡단보도
                                
                                break;

            
                            case 211: //횡단보도

                            case 201:
                                std::cout << "🏁 도착 지점" << std::endl;
                                finish = true;
                                break;

                        }

                    }    
                }else{
                    // TODO: 방향 명령 내리는 코드 추가
                    std::cout << "🧭 현재 위치 → 좌표 방향: " << angle << "도\n";
                }


            }

        } else {
            std::cout << "[❌] GPS 데이터 수신 대기 중..." << std::endl;
        }

        // 디버깅용 출력
        for (const auto& [lat, lon, turn] : parsed_coords) {
            std::cout << "위도: " << lat << ", 경도: " << lon
                      << ", turnType: " << (turn >= 0 ? std::to_string(turn) : "없음") << "\n";
        }

        usleep(1000000);  // 1초 대기
    }

    close_logger();
    // return 0 은 이 함수 내부에서는 필요 없음
}
