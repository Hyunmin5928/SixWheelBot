// src/motor_module.cpp
#include "motorCtrl.h"
#include "Lidar.h"
#include "SafeQueue.hpp"
#include <atomic>
#include <chrono>
#include <thread>
#include <limits>
#include <iostream>

// 외부에서 선언된 전역 플래그
extern std::atomic<bool> running;

// 모듈 진입점
void motor_thread(
    SafeQueue<std::vector<ScanPoint>>& lidar_q,
    SafeQueue<int>&                    cmd_q,
    std::atomic<bool>&                 running)
{
    Motor motor;
    Lidar lidar;

    while (running) {
        // 1) LiDAR 스캔
        lidar.scan_oneCycle();
        auto scanpoints = lidar.get_scanpoints();
        lidar_q.Produce(scanpoints);

        // 2) 장애물 검출 & 기본 주행
        float nearestDist  = std::numeric_limits<float>::infinity();
        float nearestAngle = 0.0f;
        for (auto &p : scanpoints) {
            if (p.angle > -20.0f && p.angle < 20.0f
             && p.range  >  0.0f
             && p.range  <  nearestDist)
            {
                nearestDist  = p.range;
                nearestAngle = p.angle;
            }
        }
        if (nearestDist < avoidDistance_trigger) {
            // 장애물 회피
            motor.curve_avoid(nearestDist, 700, nearestAngle);
        }
        else {
            // 직진
            motor.straight(500);
        }

        // 3) 상위 모듈(통신)로부터 온 명령 처리
        int cmd;
        if (cmd_q.Consume(cmd)) {
            switch (cmd) {
                case 0:  // pause
                    motor.stop();
                    std::cout << "[motor] PAUSE\n";
                    break;
                case 1:  // unlock (예시)
                    motor.curve_avoid( nearestDist, 300, 0.0f );
                    std::cout << "[motor] UNLOCK 행동\n";
                    break;
                case 2:  // return
                    motor.backoff(400);
                    std::cout << "[motor] RETURN 행동\n";
                    break;
                default:
                    break;
            }
        }

        // 20Hz 루프
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // 종료 시 모터 정지
    motor.stop();
    // 큐에 더 이상 값이 들어오지 않음을 알림
    lidar_q.Finish();
    cmd_q.Finish();
}
