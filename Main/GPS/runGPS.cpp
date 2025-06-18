#include "runGPS.h"
#include <iostream>
#include <unistd.h>

void runGPS(
    SafeQueue<std::vector<std::tuple<double,double,int>>>& dir_q,
    SafeQueue<std::pair<double,double>>&                  gps_q,
    std::atomic<bool>&                                    running)
{
    std::vector<std::tuple<double,double,int>> waypoints;
    while (running) {
        // map data 수신 시 업데이트
        std::vector<std::tuple<double,double,int>> newwp;
        if (dir_q.ConsumeSync(newwp)) {
            waypoints = std::move(newwp);
            std::cout<<"🚀 경로 업데이트: "<<waypoints.size()<<"개 지점"<<std::endl;
        }
        // 현재 GPS 위치 확인
        std::pair<double,double> pos;
        if (gps_q.ConsumeSync(pos)) {
            // pos.first, pos.second 가 현재 위도/경도
            auto [lat, lon, turn] = waypoints.front();
            double angle = GeoUtils::bearing(
                pos.first, pos.second, lat, lon);
            std::cout << "🧭 현재 위/경도: "
                    << pos.first << ", " << pos.second
                    << " → 다음 방위: " << angle
                    << "°, 턴: " << turn << "\n";
        }
        sleep(1);
    }
}
