#include "GPS_parse.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <vector>


GPS::GPS()
{
   
    memset(&_gps, 0, sizeof(_gps));


    _count = 0;

    // [✅] GPS UART 초기화
    _gps_fd = open("/dev/serial0", O_RDWR | O_NOCTTY | O_SYNC);
    if (_gps_fd >= 0) {
        struct termios tty;
        tcgetattr(_gps_fd, &tty);
        cfsetospeed(&tty, B9600);
        cfsetispeed(&tty, B9600);
        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
        tty.c_cflag |= (CLOCAL | CREAD);
        tty.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS);
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_lflag = 0;
        tty.c_oflag = 0;
        tty.c_cc[VMIN] = 1;
        tty.c_cc[VTIME] = 1;
        tcsetattr(_gps_fd, TCSANOW, &tty);
    } else {
        printf("❌ GPS UART open failed\n");
    }

    _gps_buff[0] = 0;
    _gps_len = 0;
    _utc_prev = 0.;
}


GPS::~GPS()
{
    
    if (_gps_fd > 0) {
        close(_gps_fd);
    }
}

bool GPS::GetGPSdata(sGPS* gps)
{
    int n = read(_gps_fd, &_gps_buff[_gps_len], _gps_buff_size - _gps_len);
    if (n > 0) _gps_len += n;

    if (_gps_len > 2 && (_gps_buff[_gps_len - 1] == '\n' || _gps_buff[_gps_len - 1] == '\r')) {
        _gps_buff[_gps_len] = '\0';
        printf("📡 수신된 NMEA 문장: %s\n", _gps_buff);

        char* line = strtok(_gps_buff, "\n");
        bool gngga_parsed = false;
        bool gnrmc_parsed = false;

        while (line != NULL) {
            if (strncmp(line, "$GNGGA", 6) == 0) {
                char utc[16], lat[16], ns, lon[16], ew;
                int quality = 0, sats = 0;
                float hdop = 0.0, altitude = 0.0;

                int matched = sscanf(line,
                    "$GNGGA,%15[^,],%15[^,],%c,%15[^,],%c,%d,%d,%f,%f",
                    utc, lat, &ns, lon, &ew, &quality, &sats, &hdop, &altitude);

                if (matched == 9) {
                    double hh = 0, mm = 0, ss = 0;
                    sscanf(utc, "%2lf%2lf%lf", &hh, &mm, &ss);
                    gps->utc = hh * 3600 + mm * 60 + ss;

                    double dlat = atof(lat);
                    double dlon = atof(lon);
                    gps->latitude = (int)(dlat / 100) + fmod(dlat, 100) / 60.0;
                    gps->longitude = (int)(dlon / 100) + fmod(dlon, 100) / 60.0;
                    if (ns == 'S') gps->latitude *= -1;
                    if (ew == 'W') gps->longitude *= -1;

                    gps->gps_quality = quality;
                    gps->no_sats = sats;
                    gps->hdop = hdop;
                    gps->altitude = altitude;

                    printf("✅ GNGGA 파싱 완료\n");
                    log_msg("INFO", "위도 : " + std::to_string(gps->latitude));
                    log_msg("INFO", "경도 : " + std::to_string(gps->longitude));
                    gngga_parsed = true;
                } else {
                    printf("❌ GNGGA 파싱 실패 (matched=%d)\n", matched);
                }
            }
            
            
            line = strtok(NULL, "\n");
        }

        _gps_len = 0;
        return gngga_parsed ;
    }

    if (_gps_len >= _gps_buff_size) {
        _gps_len = 0;
        printf("⚠️ GPS 버퍼 리셋됨 (오버플로우)\n");
    }

    return false;
} 