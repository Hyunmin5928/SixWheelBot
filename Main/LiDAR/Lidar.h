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

class Lidar{
    private:
        CYdLidar lidar;
        LaserScan scanData; //스캔 데이터 직접 받는 곳 삭제 절대 XXX
        std::vector<LaserPoint> usableData; //스캔 데이터  정제하여 받는 곳

        int lidar_setup();
    public:
        Lidar();
        ~Lidar();
        void scan_oneCycle();
        std::vector<LaserPoint> get_scanData();
};