#pragma once
#include "../SafeQueue.hpp"
#include <atomic>
#include "lib/cal_distance.h"
#include "lib/GPS_parse.h"
#include "lib/logger.h"
#include "lib/SerialPort.h"

void runGPS(
    SafeQueue<int>& dir_q,
    std::atomic<bool>& running);
