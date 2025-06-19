// src/lidar_module.cpp
#include <atomic>
#include <chrono>
#include <thread>
#include "../SafeQueue.hpp"
#include "Lidar.h"

// 전역 종료 플래그
extern std::atomic<bool> running;

void lidar_thread(SafeQueue<std::vector<LaserPoint>>& lidar_q);

void lidar_thread(
    SafeQueue<std::vector<LaserScan>>& lidar_q);
