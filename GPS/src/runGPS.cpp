#include <iostream>
#include "GPS_parse.h"
#include <unistd.h>  // usleep
#include "logger.h"

int main() {
    GPS gps_device;
    sGPS gps;
    // init_logger("gps.log");
    // sIMU imu;
    // init_logger("gps.log");
    std::cout << "main started\n";
    bool G_flag = false;
    //bool I_flag = false;

    //const double dt = 0.05;  // 50ms 가정 (usleep 500ms 동안 여러 번 예측 가능)
    std::cout << "1" << endl;
    while (true) {
        std::cout << "while" << endl;
        G_flag = gps_device.GetGPSdata(&gps);
        if (G_flag) {
            std::cout << "[✅] GPS 데이터 수신 성공!" << std::endl;
               
        } else {
            std::cout << "[❌] GPS 데이터 수신 대기 중..." << std::endl;
        }
     
    }
    // close_logger();
    return 0;
}
