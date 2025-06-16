#include <iostream>
#include "IMU_GPS.h"
#include <unistd.h>

int main() {
    IMU_GPS imu_gps;
    sGPS gps;
    std::cout << "main started\n";
    bool G_flag = false;

    while (true) {
        G_flag = imu_gps.GetGPSdata(&gps);
        if (G_flag) {
            std::cout << "[✅] GPS 데이터 수신 성공!" << std::endl;
        } else {
            std::cout << "[❌] GPS 데이터 수신 대기 중..." << std::endl;
        }
    }

    return 0;
}
