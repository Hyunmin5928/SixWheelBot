#pragma once
#include <iostream>
#include <cmath>
#include <vector>
#include <functional>
#include <fstream>
// #include <ctime>
#include <string>
#include "YDLidar-SDK/src/CYdLidar.h"
#include "YDLidar-SDK/core/common/ydlidar_help.h"


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
        LaserPoint get_nearPoint();
        std::vector<LaserPoint> get_scanData();
};