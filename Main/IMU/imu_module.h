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

const char* cmd = "start\n";
const char* cmd2 = "stop\n";  // 길이 5

struct ImuData {
    double roll;
    double pitch;
    double yaw;
};

class ImuModule {
public:
    ImuModule(const std::string& port,
              unsigned int baud,
              SafeQueue<ImuData>& queue);
    ~ImuModule();

    void start();
    void stop();

private:
    void run();

    CSerialPort          serial_;
    SafeQueue<ImuData>&  queue_;
    std::thread          worker_;
    std::atomic<bool>    running_{false};
    std::atomic<bool>    threadAlive_{true};
};
