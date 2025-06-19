#pragma once
#include <atomic>
#include <vector>
#include <tuple>
#include "../SafeQueue.hpp"
#include "../logger.h"
using Route = std::vector<std::tuple<double,double,int>>;

void gps_thread(
    SafeQueue<std::pair<double,double>>& gps_q,
    SafeQueue<Route>&                    map_q,
    SafeQueue<int>&                      dir_q,
    std::atomic<bool>&                   running);