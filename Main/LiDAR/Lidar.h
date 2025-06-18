#pragma once
#include "lib/CYdLidar.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <functional>
#include <core/common/ydlidar_help.h>
#include <fstream>
#include <ctime>
#include <string>

// 임시 로그
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <string>


using json = nlohmann::json;

// 임시 로그 끝

class Lidar{
    private:
        CYdLidar lidar;
        LaserScan scanData; //스캔 데이터 직접 받는 곳 삭제 절대 XXX
        std::vector<LaserPoint> usableData; //스캔 데이터  정제하여 받는 곳

        int lidar_setup();
        
        std::string SERVER_IP;
        int         SERVER_PORT;
        std::string CLIENT_IP;
        int         CLIENT_PORT;
        std::string ALLOW_IP;
        double      ACK_TIMEOUT;
        int         RETRY_LIMIT;
        std::string LOG_FILE="LiDAR/log/lidar_log.txt"; // 기본 로그 파일 경로

        int log_fd;
        int sock_fd = -1;

        int count_recentLine();

    public:
        Lidar();
        ~Lidar();
        void open_log_file();
        void close_log_file();
        std::string read_last_line();
        std::vector<LaserScan> read_log(const std::string& log_file);
        void log_msg(const std::string& level, const std::string& msg);

        void scan_oneCycle();
        std::vector<LaserPoint> get_scanData();
        void log_scanData();

       
};