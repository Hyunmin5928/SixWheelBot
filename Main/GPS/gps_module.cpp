#include "gps_module.h"
#include <chrono>
#include <thread>
#include <iostream>

extern std::vector<double> get_current_gps();

void gps_thread(SafeQueue<std::pair<double,double>>& gps_q,
                std::atomic<bool>&                    running) {
    while (running) {
        auto pos = get_current_gps();
        gps_q.Produce({pos[0], pos[1]});
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    gps_q.Finish();
}
