#include <iostream>
#include "GPS_parse.h"
#include <unistd.h>
#include "logger.h"

int main() {
    GPS gps_device;
    sGPS gps;
    init_logger("gps.log");

    std::cout << "main started\n";
    bool G_flag = false;

    while (true) {
        G_flag = gps_device.GetGPSdata(&gps);
        if (G_flag) {
            std::cout << "[✅] GPS 데이터 수신 성공!" << std::endl;
        } else {
            std::cout << "[❌] GPS | IMU 데이터 수신 대기 중..." << std::endl;
        }

        if (G_flag && I_flag) {
            imu_gps.DoGPS(gps);

            std::cout << "위도: " << gps.latitude
                      << ", 경도: " << gps.longitude
                      << ", 고도: " << gps.altitude << std::endl;
            std::cout << "속도: " << gps.velocity
                      << " km/h, Heading: " << gps.heading << std::endl;
            std::cout << "보정된 TM 좌표: X = " << imu_gps._tm_x
                      << ", Y = " << imu_gps._tm_y << std::endl;
            std::cout << "-----------------------------" << std::endl;
            std::ostringstream oss;
            oss << "위도: "    << gps.latitude
                << ", 경도: " << gps.longitude
                << ", 고도: " << gps.altitude;
            log_msg("INFO", oss.str());
        }

        usleep(500000); // 0.5초 대기
    }
    close_logger();
    return 0;
}
