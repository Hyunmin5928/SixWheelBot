// IMU/imu_module.h
#pragma once

#include <thread>
#include <atomic>
#include <string>
#include <algorithm>
#include <sstream>
#include <chrono>
#include <iostream>
#include <string.h>
#include "SafeQueue.hpp"
#include "lib/SerialPort.h"   // CSerialPort 정의
#include "../logger.h"

using util::Logger;
using util::LogLevel;

// IMU에서 뽑아낼 데이터 포맷
struct ImuData {
  double roll;
  double pitch;
  double yaw;
};

//------------------------------------------------------------------------------
// 전역 상태
//------------------------------------------------------------------------------
static CSerialPort   g_serial;
// static SafeQueue<ImuData>* g_queue = nullptr;
extern std::atomic<bool>    running;
extern std::atomic<bool>    run_imu;

// 내부적으로 돌아가는 리더 스레드를 시작합니다.
// imuInit() 호출 직후에 std::thread를 띄워 주세요.
void imureader_thread(const std::string& port,
                    unsigned int baud,
                    SafeQueue<ImuData>& dataQueue);

// struct ImuData {
//     double roll;
//     double pitch;
//     double yaw;
// };

// class ImuModule {
// public:
//     ImuModule(const std::string& port,
//               unsigned int baud,
//               SafeQueue<ImuData>& queue);
//     ~ImuModule();

//     void start();
//     void stop();

// private:
//     void run();

//     CSerialPort          serial_;
//     SafeQueue<ImuData>&  queue_;
//     std::thread          worker_;
//     std::atomic<bool>    running_{false};
//     std::atomic<bool>    threadAlive_{true};
// };
