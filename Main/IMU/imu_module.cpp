// imu_module.cpp
#include "imu_module.h"
#include <sstream>
#include <chrono>
#include <iostream>

ImuModule::ImuModule(const std::string& port,
                     unsigned int baud,
                     SafeQueue<ImuData>& queue)
  : serial_(port, baud),
    queue_(queue)
{
    // 스레드 바로 시작
    worker_ = std::thread(&ImuModule::run, this);
}

ImuModule::~ImuModule() {
    // 스레드를 깔끔히 종료
    threadAlive_ = false;
    if (worker_.joinable()) worker_.join();
}

void ImuModule::start() {
    if (!running_) {
        serial_.write("start\n");
        running_ = true;
    }
}

void ImuModule::stop() {
    if (running_) {
        serial_.write("stop\n");
        running_ = false;
    }
}

void ImuModule::run() {
    std::string line;
    bool sentStart = false, sentStop = false;

    while (threadAlive_) {
        // start() 호출 시: 한번만 전송
        if (running_ && !sentStart) {
            serial_.write("start\n");
            sentStart = true;
            sentStop  = false;
        }
        // stop() 호출 시: 한번만 전송
        if (!running_ && !sentStop) {
            serial_.write("stop\n");
            sentStop  = true;
            sentStart = false;
        }

        // 데이터 수신
        if (serial_.readLine(line, 100 /*ms timeout*/)) {
            // 동작 중지 상태에서 “Not Running” 은 무시
            if (!running_) {
                // optional: 로그로 남기려면 아래 주석 해제
                // std::cout << "[IMU] " << line << "\n";
            }
            else {
                // 예시로 “ROLL = x.xx, PITCH = y.yy, YAW = z.zz” 포맷 파싱
                ImuData d{};
                std::replace(line.begin(), line.end(), ',', ' ');
                std::istringstream iss(line);
                std::string label;
                while (iss >> label) {
                    if (label == "ROLL") {
                        iss >> label; // =
                        iss >> d.roll;
                    } else if (label == "PITCH") {
                        iss >> label; // =
                        iss >> d.pitch;
                    } else if (label == "YAW") {
                        iss >> label; // =
                        iss >> d.yaw;
                    }
                }
                queue_.push(d);
            }
        }

        // 과도한 CPU 사용 방지를 위한 짧은 대기
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
