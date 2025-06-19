#pragma once

#include <vector>
#include "SerialPort.h"

using namespace std;

struct sGPS {
    double t;
    double utc;
    int gps_quality;
    int no_sats;
    double hdop;
    double latitude, longitude, altitude;
    double heading, velocity;
};



class GPS
{
public:
    GPS();
    ~GPS();

    
    bool GetGPSdata(sGPS *gps);
    

public:
 
    sGPS _gps;
private:
    int _count;

    enum { _gps_buff_size = 4096 };
    char _gps_buff[_gps_buff_size + 1];
    int _gps_len;
    double _utc_prev;
   int _gps_fd;             // ← 라즈베리 UART로 GPS 읽기용 파일 디스크립터
    CSerialPort* _rc;        // ← 아두이노 자이로 측정값 수신용 시리얼 포트 객체
};
