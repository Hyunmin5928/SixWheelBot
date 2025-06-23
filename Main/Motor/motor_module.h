#include "motorCtrl.h"
#include "../LiDAR/lidar_module.h"
#include "../SafeQueue.hpp"
#include "../logger.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <limits>
#include <iostream>
#include <atomic>
#include <cmath>
#include <sstream>

// 외부에서 선언된 전역 플래그
extern std::atomic<bool> running;
extern int         LOG_LEVEL;
extern std::atomic<bool> running;

using util::Logger;
using util::LogLevel;

using GpsDir   = int;                        // 예: 0=정지, 1=직진, 2=회전 등

// 임계거리 / 각도 범위는 필요에 따라 조정하세요
constexpr float OBSTACLE_DISTANCE_THRESHOLD = 500.0f; // mm
constexpr float OBSTACLE_ANGLE_LIMIT        = 60.0f;  // deg
constexpr int   DEFAULT_PWM                 = 50;

// 모듈 진입점

void motor_thread(
    SafeQueue<float>&    dir_queue,
    SafeQueue<LaserPoint>& point_queue,
    SafeQueue<float>& yaw_queue,
    SafeQueue<bool>& arrive_queue
);

void motor_test_thread(
    SafeQueue<string>& cmd_queue,
    SafeQueue<LaserPoint>& point_queue,
    SafeQueue<float>& yaw_queue
);

void only_motorRotate_thread();