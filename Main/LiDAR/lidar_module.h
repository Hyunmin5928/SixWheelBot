// src/lidar_module.cpp
#include <atomic>
#include <chrono>
#include <thread>
#include "../SafeQueue.hpp"
#include "../logger.h"
#include "Lidar.h"

// 전역 종료 플래그

extern std::string          LIDAR_LOG_FILE;
extern int                  LOG_LEVEL;
extern std::atomic<bool>    running;
extern std::atomic<bool>    run_lidar;

// 시리얼 포트 객체 (전역)
// static CSerialPort g_serial;

using util::Logger;                             
using util::LogLevel;

void lidar_thread(SafeQueue<std::vector<LaserPoint>>& lidar_q);