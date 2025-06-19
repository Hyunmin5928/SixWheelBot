#include "motorCtrl.h"
#include "../SafeQueue.hpp"
#include "../logger.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <limits>
#include <iostream>
#include <atomic>

// 외부에서 선언된 전역 플래그
extern std::atomic<bool> running;
extern int         LOG_LEVEL;
extern std::atomic<bool> running;

using util::Logger;
using util::LogLevel;


// 모듈 진입점
void motor_thread(SafeQueue<std::pair<int, double>>& m_cmd_q);