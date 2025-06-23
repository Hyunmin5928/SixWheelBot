// IMU/imu_module.cpp
#include "imu_module.h"

//------------------------------------------------------------------------------
// 공용 API 구현
//------------------------------------------------------------------------------

const char cmd_start[] = "start\n";
const char cmd_stop [] = "stop\n";

void imuStart() {
    if (g_threadAlive) {
        g_serial.Write(cmd_start, int(strlen(cmd_start)));
        g_running = true;
        Logger::instance().info("imu", "[IMU]IMU start cmd sent");
    }
}

void imuStop() {
    if (g_threadAlive) {
        g_serial.Write(cmd_stop, int(strlen(cmd_stop)));
        g_running = false;
        Logger::instance().info("imu", "[IMU] IMU stop cmd sent");
    }
}

void imureader_thread(const std::string& port,
                    unsigned int baud,
                    SafeQueue<ImuData>& queue) {
                        // 딱 한 번 초기화
    if (!g_serial.Open(port.c_str(), baud)) {
        std::ostringstream oss;
        oss << "[IMU] Failed to open " << port << "@" << baud;
        Logger::instance().error("imu", oss.str());
        return;
    }
    Logger::instance().info("imu", "Serial port opened: " + port);
    g_queue = &queue;
    g_threadAlive = true;
    bool sentStart = false, sentStop = false;
    char linebuf[128];

    while (g_threadAlive) {
        // 상태 변화 시 start/stop 한 번만 보내기
        if (run_imu.load() && !sentStart) {
            g_serial.Write(cmd_start, int(strlen(cmd_start)));
            sentStop  = false;
            sentStart = true; 
            Logger::instance().info("imu", "[IMU] start cmd sent");
        } 
        if (!run_imu.load() && !sentStop) {
            g_serial.Write(cmd_stop, int(strlen(cmd_stop)));
            sentStop  = true; 
            sentStart = false;
            Logger::instance().info("imu", "[IMU] stop cmd sent");
        }

        // 한 줄 읽기 (timeout 100ms)
        if (g_serial.ReadLine(linebuf, sizeof(linebuf))) {
            if (run_imu.load()) {
                // "ROLL = x.xx, PITCH = y.yy, YAW = z.zz" 파싱
                std::string s(linebuf);
                std::replace(s.begin(), s.end(), ',', ' ');
                std::istringstream iss(s);
                std::string label;
                ImuData d{};
                while (iss >> label) {
                    if (label == "ROLL") {
                        iss >> label >> d.roll;
                    } else if (label == "PITCH") {
                        iss >> label >> d.pitch;
                    } else if (label == "YAW") {
                        iss >> label >> d.yaw;
                    }
                    }
                std::ostringstream oss;
                oss << "[IMU] Roll : "<< std::to_string(d.roll) << ",Pitch : " << std::to_string(d.pitch) << ",Yaw : "<< std::to_string(d.yaw);
                // 큐로 전달
                g_queue->Produce(std::move(d));
            }
        }
        // CPU 과다 사용 방지
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    g_serial.Close();
    Logger::instance().info("imu", "[IMU] Serial closed");
}

// ImuModule::ImuModule(const std::string& port,
//                      unsigned int baud,
//                      SafeQueue<ImuData>& queue)
//   : queue_(queue)
// {
//     // 시리얼 포트 열기
//     if (!serial_.Open(port.c_str(), baud)) {
//         std::cerr << "[IMU] Failed to open " << port << "@" << baud << "\n";
//     }
//     // 백그라운드 스레드 시작
//     worker_ = std::thread(&ImuModule::run, this);
// }

// ImuModule::~ImuModule() {
//     // 스레드 종료 플래그
//     threadAlive_ = false;
//     if (worker_.joinable()) worker_.join();
//     serial_.Close();
// }

// void ImuModule::start() {
//     if (!running_) {
//         serial_.Write(cmd, int(strlen(cmd)));
//         running_ = true;
//     }
// }

// void ImuModule::stop() {
//     if (running_) {
//         serial_.Write(cmd2, int(strlen(cmd2)));
//         running_ = false;
//     }
// }

// void ImuModule::run() {
//     std::string line;
//     bool sentStart = false, sentStop = false;

//     while (threadAlive_) {
//         // 상태 변화 시 한 번만 전송
//         if (running_ && !sentStart) {
//             serial_.Write(cmd, int(strlen(cmd)));
//             sentStart = true;
//             sentStop  = false;
//         }
//         if (!running_ && !sentStop) {
//             serial_.Write(cmd2, int(strlen(cmd2)));
//             sentStop  = true;
//             sentStart = false;
//         }

//         // 시리얼에서 한 라인 읽기 (timeout 100ms)
//         char buf[128];
//         if (serial_.ReadLine(buf, sizeof(buf))) {
//             if (running_) {
//                 // "ROLL = x.xx, PITCH = y.yy, YAW = z.zz" 포맷 파싱
//                 std::replace(line.begin(), line.end(), ',', ' ');
//                 std::istringstream iss(line);
//                 std::string label;
//                 ImuData d{};
//                 while (iss >> label) {
//                     if (label == "ROLL") {
//                         iss >> label; // '='
//                         iss >> d.roll;
//                     }
//                     else if (label == "PITCH") {
//                         iss >> label;
//                         iss >> d.pitch;
//                     }
//                     else if (label == "YAW") {
//                         iss >> label;
//                         iss >> d.yaw;
//                     }
//                 }
//                 queue_.Produce(std::move(d));  // SafeQueue 에 데이터 푸시
//             }
//             // running_ == false 면 "Not Running" 무시
//         }

//         // CPU 부하 완화를 위해 짧게 대기
//         std::this_thread::sleep_for(std::chrono::milliseconds(1));
//     }
// }
