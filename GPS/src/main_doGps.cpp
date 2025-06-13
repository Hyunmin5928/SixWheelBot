/*#include <iostream>
#include "IMU_GPS.h"
#include <unistd.h>  // usleep

int main() {
    IMU_GPS imu_gps;
    sGPS gps;
	printf("main\n");
    bool G_flag = false;
    while (true) {
        G_flag = imu_gps.GetGPSdata(&gps);
        if (G_flag) {
            std::cout << "[✅] GPS 데이터 수신 성공!" << std::endl;
		    printf("aa\n");
            imu_gps.DoGPS(gps);

            std::cout << "위도: " << gps.latitude
                      << ", 경도: " << gps.longitude
                      << ", 고도: " << gps.altitude << std::endl;
            std::cout << "속도: " << gps.velocity
                      << " km/h, Heading: " << gps.heading << std::endl;
            std::cout << "보정된 TM 좌표: X = " << imu_gps._tm_x
                      << ", Y = " << imu_gps._tm_y << std::endl;
            std::cout << "-----------------------------" << std::endl;
        } else {
            std::cout << "[❌] GPS 데이터 수신 대기 중..." << std::endl;
        }

        usleep(500000); // 0.5초 대기
    }
    return 0;
}

*/

/*
#include <iostream>
#include "IMU_GPS.h"
#include <unistd.h>  // usleep

int main() {
    IMU_GPS imu_gps;
    sGPS gps;
    sIMU imu;  // <-- IMU 구조체 추가

    std::cout << "main started\n";
    bool G_flag = false;
    bool I_flag = false;

    while (true) {
        // IMU 데이터 수신
        I_flag = imu_gps.GetIMUdata(&imu);
        if (I_flag) {
            std::cout << "🌀 자이로: ["
                      << imu.rpy[0] << ", "
                      << imu.rpy[1] << ", "
                      << imu.rpy[2] << "]" << std::endl;
        } 
        else {
            std::cout << "❌ IMU 데이터 없음" << std::endl;
        }

        // GPS 데이터 수신
        G_flag = imu_gps.GetGPSdata(&gps);
        if (G_flag) {
            std::cout << "[✅] GPS 데이터 수신 성공!" << std::endl;}
        else{
             std::cout << "[❌] GPS | IMU 데이터 수신 대기 중..." << std::endl;
        }
        if(G_flag&&I_flag){
            imu_gps.DoGPS(gps);

            std::cout << "위도: " << gps.latitude
                      << ", 경도: " << gps.longitude
                      << ", 고도: " << gps.altitude << std::endl;
            std::cout << "속도: " << gps.velocity
                      << " km/h, Heading: " << gps.heading << std::endl;
            std::cout << "보정된 TM 좌표: X = " << imu_gps._tm_x
                      << ", Y = " << imu_gps._tm_y << std::endl;
            std::cout << "-----------------------------" << std::endl;
        } 
       
    

        usleep(500000); // 0.5초 대기
    }

    return 0;
}*/

#include <iostream>
#include "IMU_GPS.h"
#include <unistd.h>  // usleep
// Logging.cpp
#include <chrono>
#include <ctime>
#include <fcntl.h>
#include <iomanip>
#include <sstream>

// 전역 log_fd
static int log_fd = -1;

void init_logger(const std::string& log_file) {
    log_fd = open(log_file.c_str(), O_CREAT | O_APPEND | O_WRONLY, 0644);
    if (log_fd < 0) {
        std::cerr << "ERROR: Cannot open log file: " << log_file << "\n";
    }
}

void close_logger() {
    if (log_fd >= 0) {
        close(log_fd);
        log_fd = -1;
    }
}

void log_msg(const std::string& level, const std::string& msg) {
    if (log_fd < 0) return;  // 로거가 초기화되지 않았다면 무시

    // 현재 시간 문자열로 포맷
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S")
        << " [" << level << "] " << msg << "\n";
    // 파일 디스크립터에 기록
    write(log_fd, oss.str().c_str(), oss.str().size());
}

int main() {
    IMU_GPS imu_gps;
    sGPS gps;
    sIMU imu;
    init_logger("gps.log");
    std::cout << "main started\n";
    bool G_flag = false;
    bool I_flag = false;

    const double dt = 0.05;  // 50ms 가정 (usleep 500ms 동안 여러 번 예측 가능)

    while (true) {
        // IMU 데이터 수신 및 예측
        I_flag = imu_gps.GetIMUdata(&imu);
        if (I_flag) {
            std::cout << "🌀 자이로: [" << imu.rpy[0] << ", " << imu.rpy[1] << ", " << imu.rpy[2] << "]" << std::endl;

            // ✅ 예측 단계 추가
            imu_gps.DoIMU(imu, dt);

            // ✅ 예측된 위치 출력
            std::cout << "[📍 IMU 예측 위치] "
                      << "X: " << imu_gps._p[0]
                      << ", Y: " << imu_gps._p[1]
                      << ", Z: " << imu_gps._p[2] << std::endl;
        } else {
            std::cout << "❌ IMU 데이터 없음" << std::endl;
        }

        // GPS 데이터 수신
        G_flag = imu_gps.GetGPSdata(&gps);
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

