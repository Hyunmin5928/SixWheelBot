#include "runGPS.h"
#include <iostream>
#include <unistd.h>
#include "../logger.h"

void runGPS(
    SafeQueue<int>&                                       dir_q,
    SafeQueue<std::pair<double,double>>&                  gps_q,
    std::atomic<bool>&                                    running)
{
    while (running) {
        // 1) dir_queue 에서 정수 코드만 꺼내 쓰기
        int code;
        if (dir_q.ConsumeSync(code)) {
            std::cout << "▶ RunGPS: direction code = "
                      << code << std::endl;
        }

        // 2) gps_queue 에서 현재 위치 꺼내서 출력
        std::pair<double,double> pos;
        if (gps_q.ConsumeSync(pos)) {
            std::cout << "🗺️ Current GPS: lat=" 
                      << pos.first << ", lon="
                      << pos.second << std::endl;
        }

        sleep(1);
    }
}