#include <core/base/timer.h>
#include <core/common/ydlidar_help.h>
#include <../../home/lbunge-note/Desktop/SixWheelBot/Motor/motorCtrl.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>

class Lidar{
    Motor* motor;
public:
    Lidar(Motor* m);
    ~Lidar();
    int start();
};