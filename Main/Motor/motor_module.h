#pragma once

#include <atomic>
#include <chrono>
#include <thread>
#include <sstream>
#include <iostream>
#include <string.h>
#include <cmath>
#include "../SafeQueue.hpp"
#include "../logger.h"
#include "../LiDAR/YDLidar-SDK/core/common/ydlidar_def.h"
#include "lib/SerialPort.h"

extern std::string          COMMAND_LOG_FILE;
extern int                  LOG_LEVEL;
extern std::atomic<bool>    running;
extern std::atomic<bool>    run_motor;


// 시리얼 포트 객체 (전역)
static CSerialPort g_serial;

// 임계거리 / 각도 범위는 필요에 따라 조정하세요
constexpr float OBSTACLE_DISTANCE_THRESHOLD = 500.0f; // mm
constexpr float OBSTACLE_ANGLE_LIMIT        = 60.0f;  // deg
constexpr int   DEFAULT_PWM                 = 50;

using util::Logger;                             
using util::LogLevel;

void motor_thread(
    const std::string& port,
    unsigned int baud,
    SafeQueue<float>& dir_queue,
    SafeQueue<LaserPoint>& point_queue
);