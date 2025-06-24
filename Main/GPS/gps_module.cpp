#include "gps_module.h"
#include <chrono>
#include <thread>
#include <cmath>
#include <iostream>
#include "cal_distance.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEG2RAD(deg) ((deg) * M_PI / 180.0)
#define RAD2DEG(rad) ((rad) * 180.0 / M_PI)

static double haversine(double lat1, double lon1, double lat2, double lon2) {
    constexpr double R = 6371000.0;
    auto toRad = [](double deg){ return deg * M_PI / 180.0; };
    double dLat = toRad(lat2 - lat1);
    double dLon = toRad(lon2 - lon1);
    double a = std::sin(dLat/2)*std::sin(dLat/2)
             + std::cos(toRad(lat1))*std::cos(toRad(lat2))
             * std::sin(dLon/2)*std::sin(dLon/2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    double result = R * c;
    Logger::instance().info("gps", "[nav] Calc Result : " + std::to_string(result));
    return R * c;
}

    double bearing(double lat1, double lon1, double lat2, double lon2) {
        double phi1 = DEG2RAD(lat1);
        double phi2 = DEG2RAD(lat2);
        double delta_lambda = DEG2RAD(lon2 - lon1);

        double y = std::sin(delta_lambda) * std::cos(phi2);
        double x = std::cos(phi1) * std::sin(phi2) -
                   std::sin(phi1) * std::cos(phi2) * std::cos(delta_lambda);

        double theta = std::atan2(y, x);
        return fmod((RAD2DEG(theta) + 360.0), 360.0);  // 0~360도 범위
    }

void navigation_thread(
    SafeQueue<std::vector<Waypoint>>& map_q,
    SafeQueue<float>& dir_queue
) {
    // 1) MAP 수신 
    std::vector<Waypoint> path;
    if (!map_q.ConsumeSync(path)) {
        std::cerr << "[nav] map data not received\n";
        return;
    }
    // Logger::instance().info("gps", "[navigation_thread] GPS navigation thread start");
    size_t idx = 0;
    const double threshold = 1.0;  // m
    GPS gpsSensor;
    sGPS raw;
    bool finish = false;
    bool flag = true; // 복귀 or 출발 

    // 2) 네비게이션 루프
    while (running.load()) {
        if (!run_gps.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }
        // Logger::instance().info("gps", "[navigation_thread] Running True");
        float dir2=0;
        if(finish){
            break;
        }
        // GPS 데이터 수신
        if (!gpsSensor.GetGPSdata(&raw)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }

        double lat = raw.latitude;
        double lon = raw.longitude;

        if(flag){ // 목적지로 가는 경우 (1. 출발지 -> 사용자 ) 
            run_motor.store(true);
            if (idx < path.size()) {
            auto [wlat, wlon, dir] = path[idx];
            double dist = haversine(lat, lon, wlat, wlon);
            Logger::instance().info("gps", "[navigation_thread] Path Calc");
            if (dist <= threshold) {
                if(dir>0){
                    switch (dir) {
                        case 12: // 좌회전
                            Logger::instance().info("gps", "[navigation_thread] case 12");
                            dir2 = -90;
                            break;
                        case 212: //좌회전 + 횡단보도
                            Logger::instance().info("gps", "[navigation_thread] case 212");
                            dir2 = -90;
                            break;
                        case 16: // 8시 방향 좌회전
                            Logger::instance().info("gps", "[navigation_thread] case 16");
                            dir2 = -120;
                            break;
                        case 214: //8시 방향 + 횡단보도
                            Logger::instance().info("gps", "[navigation_thread] case 214");
                            dir2 = -120;
                            break;
                        case 17: // 10시 방향 좌회전
                            Logger::instance().info("gps", "[navigation_thread] case 17");
                            dir2 = -60;
                            break;
                        case 215: //10시 방향 + 횡단보도
                            Logger::instance().info("gps", "[navigation_thread] case 215");
                            dir2 = -60;
                            break;
                        case 13: // 우회전
                            Logger::instance().info("gps", "[navigation_thread] case 13");
                            dir2 = 90;
                            break;
                        case 213: //우회전 + 횡단보도 
                            Logger::instance().info("gps", "[navigation_thread] case 213");
                            dir2 = 90;
                            break;
                        case 18: // 2시 방향 우회전
                            Logger::instance().info("gps", "[navigation_thread] case 18");
                            dir2 = 60;
                            break;
                        case 216: //2시 방향 + 횡단보도
                            Logger::instance().info("gps", "[navigation_thread] case 216");
                            dir2 = 60;
                            break;
                        case 19: // 4시 방향 우회전
                            Logger::instance().info("gps", "[navigation_thread] case 19");
                            dir2 = 120;
                            break;
                        case 217: //4시 방향 + 횡단보도
                            Logger::instance().info("gps", "[navigation_thread] case 217");
                            dir2 = 120;
                            break;
                        case 211: //횡단보도
                            Logger::instance().info("gps", "[navigation_thread] case 211");
                            break;
                        case 201:
                            std::cout << "🏁 도착 지점" << std::endl;
                            Logger::instance().info("gps", "[navigation_thread] case 201");
                            dir2 = 0;
                            break;
                        }
                    }
                    // 도달: 방향 코드 전송
                    dir_queue.Produce(std::move(dir2));
                    idx++;
                } else {
                    // (선택) 전진 명령 보내기
                    // m_cmd_q.Produce(FORWARD_CMD);
                    double angle = bearing(lat, lon, wlat, wlon);
                    if(angle>45){
                        dir_queue.Produce(std::move(angle));
                    }
                }
            }   else {
                // 경로 완료: PAUSE
                Logger::instance().info("gps", "[navigation_thread] Complete Path");
                //finish = true;
                //dir_queue.Produce(1000);
                break;
             }
             if(finish){ //목적지 도착 -> 정지
             run_motor.store(false);
              flag = false;
              idx = path.size()-1;
              finish = false;
            }
        }
        else{  // 복귀하는 경우 (2. 사용자 -> 출발지)
            run_motor.store(true);
            if (idx > -1) {
            auto [wlat, wlon, dir] = path[idx];
            double dist = haversine(lat, lon, wlat, wlon);
            Logger::instance().info("gps", "[navigation_thread] Path Calc");
            if (dist <= threshold) {
                if(dir>0){
                    switch (dir) {
                        case 12: // 좌회전
                            Logger::instance().info("gps", "[navigation_thread] case 12");
                            dir2 = 90;
                            break;
                        case 212: //좌회전 + 횡단보도
                            Logger::instance().info("gps", "[navigation_thread] case 212");
                            dir2 = 90;
                            break;
                        case 16: // 8시 방향 좌회전
                            Logger::instance().info("gps", "[navigation_thread] case 16");
                            dir2 = -120;
                            break;
                        case 214: //8시 방향 + 횡단보도
                            Logger::instance().info("gps", "[navigation_thread] case 214");
                            dir2 = -120;
                            break;
                        case 17: // 10시 방향 좌회전 -> 두시 방향 우회전
                            Logger::instance().info("gps", "[navigation_thread] case 17");
                            dir2 = 60;
                            break;
                        case 215: //10시 방향 + 횡단보도
                            Logger::instance().info("gps", "[navigation_thread] case 215");
                            dir2 = 60;
                            break;
                        case 13: // 우회전
                            Logger::instance().info("gps", "[navigation_thread] case 13");
                            dir2 = -90;
                            break;
                        case 213: //우회전 + 횡단보도 
                            Logger::instance().info("gps", "[navigation_thread] case 213");
                            dir2 = -90;
                            break;
                        case 18: // 2시 방향 우회전
                            Logger::instance().info("gps", "[navigation_thread] case 18");
                            dir2 = -60;
                            break;
                        case 216: //2시 방향 + 횡단보도
                            Logger::instance().info("gps", "[navigation_thread] case 216");
                            dir2 = -60;
                            break;
                        case 19: // 4시 방향 우회전
                            Logger::instance().info("gps", "[navigation_thread] case 19");
                            dir2 = 120;
                            break;
                        case 217: //4시 방향 + 횡단보도
                            Logger::instance().info("gps", "[navigation_thread] case 217");
                            dir2 = 120;
                            break;
                        case 211: //횡단보도
                            Logger::instance().info("gps", "[navigation_thread] case 211");
                            break;
                        case 201:
                            std::cout << "🏁 도착 지점" << std::endl;
                            Logger::instance().info("gps", "[navigation_thread] case 201");
                            dir2 = 0;
                            finish = true;
                            break;
                        }
                    }
                    // 도달: 방향 코드 전송
                    dir_queue.Produce(std::move(dir2));
                    idx--;
                } else {
                    // (선택) 전진 명령 보내기
                    // m_cmd_q.Produce(FORWARD_CMD);
                    double angle = bearing(lat, lon, wlat, wlon);
                    if(angle>45){
                        dir_queue.Produce(std::move(angle));
                    }
                }
            }   else {
                // 경로 완료: PAUSE
                Logger::instance().info("gps", "[navigation_thread] Complete Path");
                finish = true;
                //dir_queue.Produce(1000);
                run_motor.store(false);
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

    }
    dir_queue.Finish();
}

void gps_reader_thread(
    SafeQueue<GpsPos>& gps_q
) {
    GPS gpsSensor;
    sGPS  raw;
    // 읽기 루프
    while (running.load()) {
        // Logger::instance().info("gps", "[gps_reader_thread] Running True");
        if (!run_gps.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }
        // 하드웨어에서 유효한 GNGGA 메시지 하나 읽어들이면 true 반환
        // Logger::instance().info("gps", "[gps_reader_thread] GPS reader thread start");
        if (gpsSensor.GetGPSdata(&raw)) {
            double lat = raw.latitude;
            double lon = raw.longitude;

            // 위치 큐에 푸시
            gps_q.Produce({lat, lon});

            // (선택) 로그 출력
            std::string msg =
                "[gps_reader_thread] GPS: 위도=" + std::to_string(lat) +
                ", 경도=" + std::to_string(lon);
            Logger::instance().info("gps", msg);
        }

        // 너무 빡빡하게 읽지 않도록 잠깐 대기 (예: 200ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // 종료 신호 시 큐에도 끝 알리기
    gps_q.Finish();
}