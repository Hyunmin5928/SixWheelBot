#pragma once
#include "../SafeQueue.hpp"
#include "../logger.h"
#include <atomic>
#include "lib/cal_distance.h"
#include "lib/GPS_parse.h"
#include "lib/SerialPort.h"

void runGPS(
    SafeQueue<int>& dir_q,
    SafeQueue<std::pair<double,double>>& gps_q,
    std::atomic<bool>& running);
