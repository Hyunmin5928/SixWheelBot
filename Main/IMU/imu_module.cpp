// IMU/imu_module.cpp
#include "imu_module.h"

ImuModule::ImuModule(const std::string& port,
                     unsigned int baud,
                     SafeQueue<ImuData>& queue)
  : queue_(queue)
{
    // 시리얼 포트 열기
    if (!serial_.Open(port.c_str(), baud)) {
        std::cerr << "[IMU] Failed to open " << port << "@" << baud << "\n";
    }
    // 백그라운드 스레드 시작
    worker_ = std::thread(&ImuModule::run, this);
}

ImuModule::~ImuModule() {
    // 스레드 종료 플래그
    threadAlive_ = false;
    if (worker_.joinable()) worker_.join();
    serial_.Close();
}

void ImuModule::start() {
    if (!running_) {
        serial_.Write(cmd, int(strlen(cmd)));
        running_ = true;
    }
}

void ImuModule::stop() {
    if (running_) {
        serial_.Write(cmd2, int(strlen(cmd2)));
        running_ = false;
    }
}

void ImuModule::run() {
    std::string line;
    bool sentStart = false, sentStop = false;

    while (threadAlive_) {
        // 상태 변화 시 한 번만 전송
        if (running_ && !sentStart) {
            serial_.Write(cmd, int(strlen(cmd)));
            sentStart = true;
            sentStop  = false;
        }
        if (!running_ && !sentStop) {
            serial_.Write(cmd2, int(strlen(cmd2)));
            sentStop  = true;
            sentStart = false;
        }

        // 시리얼에서 한 라인 읽기 (timeout 100ms)
        char buf[128];
        if (serial_.ReadLine(buf, sizeof(buf))) {
            if (running_) {
                // "ROLL = x.xx, PITCH = y.yy, YAW = z.zz" 포맷 파싱
                std::replace(line.begin(), line.end(), ',', ' ');
                std::istringstream iss(line);
                std::string label;
                ImuData d{};
                while (iss >> label) {
                    if (label == "ROLL") {
                        iss >> label; // '='
                        iss >> d.roll;
                    }
                    else if (label == "PITCH") {
                        iss >> label;
                        iss >> d.pitch;
                    }
                    else if (label == "YAW") {
                        iss >> label;
                        iss >> d.yaw;
                    }
                }
                queue_.Produce(std::move(d));  // SafeQueue 에 데이터 푸시
            }
            // running_ == false 면 "Not Running" 무시
        }

        // CPU 부하 완화를 위해 짧게 대기
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
