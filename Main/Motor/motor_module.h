// #include "motorCtrl.h"
#include "../LiDAR/lidar_module.h"
#include "../SafeQueue.hpp"
#include "../logger.h"
#include "lib/SerialPort.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <limits>
#include <iostream>
#include <atomic>
#include <cmath>
#include <sstream>

// 외부에서 선언된 전역 플래그
extern int         LOG_LEVEL;
extern std::atomic<bool>    running;
extern std::atomic<bool>    run_motor;
using util::Logger;
using util::LogLevel;

static CSerialPort   g_serial;

// 임계거리 / 각도 범위는 필요에 따라 조정하세요
constexpr float OBSTACLE_DISTANCE_THRESHOLD = 500.0f; // mm
constexpr float OBSTACLE_ANGLE_LIMIT        = 60.0f;  // deg
constexpr int   DEFAULT_PWM                 = 50;

// 모듈 진입점

void motor_thread(
    const std::string& port,
    unsigned int baud,
    SafeQueue<float>&    dir_queue,
    SafeQueue<LaserPoint>& point_queue
);
