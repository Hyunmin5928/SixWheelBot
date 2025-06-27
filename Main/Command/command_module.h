#include <atomic>
#include <chrono>
#include <thread>
#include "../Motor/motor_module.h"
#include "../SafeQueue.hpp"
#include "../logger.h"

extern std::string          COMMAND_LOG_FILE;
extern int                  LOG_LEVEL;
extern std::atomic<bool>    running;
extern std::atomic<bool>    run_command;


// 시리얼 포트 객체 (전역)
//static CSerialPort g_serial;

// 임계거리 / 각도 범위는 필요에 따라 조정하세요
constexpr float OBSTACLE_DISTANCE_THRESHOLD = 500.0f; // mm
constexpr float OBSTACLE_ANGLE_LIMIT        = 60.0f;  // deg
constexpr int   DEFAULT_PWM                 = 50;

using util::Logger;                             
using util::LogLevel;

void command_thread(
    SafeQueue<float> dir_queue,
    SafeQueue<LaserPoint>& point_queue,
    SafeQueue<ImuData>& imu_queue.
    SafeQueue<std::string> cmd_queue
);