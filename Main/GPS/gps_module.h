#pragma once
#include "../SafeQueue.hpp"
#include <atomic>
#include <utility>

void gps_thread(
    SafeQueue<std::pair<double,double>>& gps_q,
    std::atomic<bool>&                    running);

