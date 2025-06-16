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
            std::cout << "[❌] GPS 데이터 수신 대기 중..." << std::endl;
        }
    }

    close_logger();
    return 0;
}
