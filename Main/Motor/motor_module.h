#include "motorCtrl.h"
// #include "../LiDAR/lidar_module.h"
//#include "../IMU/imu_module.h"
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
#include <string>

// 외부에서 선언된 전역 플래그
extern int         LOG_LEVEL;
extern std::atomic<bool>    running;
extern std::atomic<bool>    run_motor;
using util::Logger;
using util::LogLevel;

// 모듈 진입점

void motor_thread(
    SafeQueue<std::string>&    cmd_queue,
    SafeQueue<LaserPoint> avoid_queue
);

