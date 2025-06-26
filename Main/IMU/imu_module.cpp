// IMU/imu_module.cpp
#include "imu_module.h"
#include <pthread.h>

//------------------------------------------------------------------------------
// 공용 API 구현
//------------------------------------------------------------------------------

static constexpr const char cmd_start[] = "start\n";
static constexpr const char cmd_stop [] = "stop\n";

void imureader_thread(const std::string& port,
                    unsigned int baud,
                    SafeQueue<ImuData>& queue) {
                        // 딱 한 번 초기화
    pthread_setname_np(pthread_self(), "[THREAD]IMU_READ");
    if (!g_serial.Open(port.c_str(), baud)) {
        std::ostringstream oss;
        oss << "[IMU] Failed to open " << port << "@" << baud;
        Logger::instance().error("imu", oss.str());
        queue.Finish();
        return;
    }
    Logger::instance().info("imu", "[IMU] Serial port opened: " + port);
    bool sentStart = false;
    bool sentStop  = false;
    char linebuf[128];

    while (running.load()) {
        // 상태 변화 시 start/stop 한 번만 보내기
        if (run_imu.load()) {
            if(!sentStart) {
                g_serial.Write(cmd_start, sizeof(cmd_start) - 1);
                sentStop  = false;
                sentStart = true; 
                Logger::instance().info("imu", "[IMU] start cmd sent");
            }
        } else {
            if(!sentStop) {
            g_serial.Write(cmd_stop, sizeof(cmd_stop) - 1);
            sentStop  = true; 
            sentStart = false;
            Logger::instance().info("imu", "[IMU] stop cmd sent");
            }
        }

        // 한 줄 읽기 (timeout 100ms)
        if (run_imu.load()) {
            if (g_serial.ReadLine(linebuf, sizeof(linebuf)) && run_imu.load()) {
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
                Logger::instance().info("imu", oss.str());
                queue.Produce(std::move(d));
        }

        }
        // CPU 과다 사용 방지
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    g_serial.Close();
    Logger::instance().info("imu", "[IMU] Serial closed");
    queue.Finish();
}

void stop_imu(){
    
}