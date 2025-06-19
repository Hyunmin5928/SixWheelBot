#include "gps_module.h"
#include <chrono>
#include <thread>
#include <cmath>
#include <iostream>

void gps_thread(
    SafeQueue<std::pair<double,double>>& gps_q,
    SafeQueue<Route>&                    map_q,
    SafeQueue<int>&                      dir_q,
    std::atomic<bool>&                   running)
{
    // 1) map data 블록킹 수신
    Route route;
    if (!map_q.ConsumeSync(route)) return;
    std::cout << "📍 Map loaded, " << route.size() << " points\n";

    const double PROX_M = 2.0; // 문턱 거리 (m)

    // 2) 메인 루프: 현재 위치 읽고, 각 웨이포인트 근처면 dir_q에 코드만
    while (running) {
        std::pair<double,double> pos;
        if (!gps_q.ConsumeSync(pos)) {
            // running==false로 Finish된 경우 루프 탈출
            break;
        }
        gps_q.Produce(std::move(pos));

        for (auto& wp : route) {
            auto [lat, lon, code] = wp;
            double dx = (pos.first - lat)*111000.0;
            double dy = (pos.second - lon)*111000.0;
            if (std::sqrt(dx*dx + dy*dy) <= PROX_M) {
                dir_q.Produce(std::move(code));
                std::cout << "▶ dir code " << code 
                          << " queued\n";
                break;
            }
        }
        std::this_thread::sleep_for(
          std::chrono::seconds(1));
    }

    gps_q.  Finish();
    map_q.  Finish();
    dir_q.  Finish();
}