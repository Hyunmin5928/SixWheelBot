#pragma once

#include "SerialPort.h"
#include <vector>

using namespace std;



struct sIMU {
    double t;
    double gyro[3];
    double accl[3];
    double rpy[3];  // 추가: Roll, Pitch, Yaw 값 포함 시
};

class IMU_GPS
{
public:
    IMU();
    ~IMU();

public:
    sIMU _imu;

private:


    // 포트 분리 시 필요한 구성
    CSerialPort *_imu_serial;  // IMU 전용 시리얼
            // ← 라즈베리 UART로 GPS 읽기용 파일 디스크립터
    CSerialPort* _rc;        // ← 아두이노 자이로 측정값 수신용 시리얼 포트 객체
};
