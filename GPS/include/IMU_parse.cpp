#include "SerialPort.h"
#include <vector>
#include "IMU_parse.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>



IMU::IMU()
{
    memset(&_imu, 0, sizeof(_imu));
    //_count = 0;

    // [❌] IMU 초기화 생략 또는 임시 주석 처리
    
    _rc = new CSerialPort();
    if (!_rc->Open("/dev/ttyUSB0", 115200)) {
        printf("❌ IMU Serial open failed\n");
        return;  // ← 이 부분도 임시로 제거
    }
    _rc->SetTimeout(100);


}


IMU::~IMU()
{
    if (_rc) {
        _rc->Close();
        delete _rc;
    }
   
}



bool IMU::GetIMUdata(sIMU* imu)
{
    char line[256];
    if (_rc->ReadLine(line, sizeof(line))) {
        sscanf(line, "%lf %lf %lf %lf %lf %lf %lf %lf %lf",
               &imu->gyro[0], &imu->gyro[1], &imu->gyro[2],
               &imu->accl[0], &imu->accl[1], &imu->accl[2],
               &imu->rpy[0],  &imu->rpy[1],  &imu->rpy[2]);
          // 🔍 디버깅 출력
        std::cout << "[IMU 수신됨] Gyro: ["
                  << imu->gyro[0] << ", "
                  << imu->gyro[1] << ", "
                  << imu->gyro[2] << "]  "
                  << "Acc: ["
                  << imu->accl[0] << ", "
                  << imu->accl[1] << ", "
                  << imu->accl[2] << "]  "
                  << "RPY: ["
                  << imu->rpy[0] << ", "
                  << imu->rpy[1] << ", "
                  << imu->rpy[2] << "]" << std::endl;
        return true;
    }
    
    return false;
}

