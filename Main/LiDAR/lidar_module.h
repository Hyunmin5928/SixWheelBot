// src/lidar_module.cpp
#include <atomic>
#include <chrono>
#include <thread>
#include "../SafeQueue.hpp"
#include "../logger.h"
#include "Lidar.h"
#include <sstream>
#include <fstream>
// #include "YDLidar-SDK/src/CYdLidar.h"
// #include "YDLidar-SDK/core/common/ydlidar_help.h"

// 전역 종료 플래그

extern std::string          LIDAR_LOG_FILE;
extern int                  LOG_LEVEL;
extern std::atomic<bool>    running;
extern std::atomic<bool>    run_lidar;

extern SafeQueue<std::vector<LaserPoint>> raw_scan_queue;

// 시리얼 포트 객체 (전역)
// static CSerialPort g_serial;

using util::Logger;                             
using util::LogLevel;

void lidar_producer();
void lidar_consumer(SafeQueue<std::vector<LaserPoint>>& in_q,
                    SafeQueue<LaserPoint>& out_q);

// void lidar_thread(SafeQueue<LaserPoint>& lidar_q);