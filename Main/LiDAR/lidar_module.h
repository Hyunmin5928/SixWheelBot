// src/lidar_module.cpp
#include <atomic>
#include <chrono>
#include <thread>
#include "../SafeQueue.hpp"
#include "../logger.h"
#include "Lidar.h"

// 전역 종료 플래그

extern std::string GPS_LOG_FILE;
extern int         LOG_LEVEL;
extern std::atomic<bool> running;

using util::Logger;
using util::LogLevel;

void lidar_thread(SafeQueue<std::vector<LaserPoint>>& lidar_q);

void lidar_thread(
    SafeQueue<std::vector<LaserScan>>& lidar_q);
