#include "motorCtrl.h"
#include "../LiDAR/Lidar.h"
#include "../SafeQueue.hpp"
#include <atomic>
#include <chrono>
#include <thread>
#include <limits>
#include <iostream>
#include <atomic>

// 외부에서 선언된 전역 플래그
extern std::atomic<bool> running;

void motor_thread(SafeQueue<int>& lidar_q,
                  SafeQueue<int>& cmd_q);

// 모듈 진입점
void motor_thread(
    SafeQueue<std::vector<LaserPoint>>& lidar_q,
    SafeQueue<int>&                    cmd_q);