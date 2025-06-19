#include "motorCtrl.h"
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
using LidarObs = std::pair<float,float>;     // {range_cm, angle_deg}

// 임계거리 / 각도 범위는 필요에 따라 조정하세요
constexpr float OBSTACLE_DISTANCE_THRESHOLD = 800.0f; // cm
constexpr float OBSTACLE_ANGLE_LIMIT        = 20.0f;  // deg
constexpr int   DEFAULT_PWM                 = 700;

// 모듈 진입점
// void motor_thread(SafeQueue<std::pair<int, double>>& m_cmd_q);

void motor_thread(
    SafeQueue<GpsDir>& dir_queue,
    SafeQueue<LidarObs>& lidar_queue
);