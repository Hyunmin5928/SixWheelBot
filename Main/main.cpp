#include <thread>
#include <atomic>
#include <csignal>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <nlohmann/json.hpp>
#include "SafeQueue.hpp"
#include "Communication/comm_module.h"
#include "GPS/gps_module.h"

using json = nlohmann::json;

std::atomic<bool> running{true};
int log_fd;

void handle_sigint(int) { running = false; }

void load_config(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs) { std::cerr<<"설정 파일 열기 실패: "<<path<<"\n"; std::exit(1); }
    json cfg; ifs >> cfg;
    std::string LOG_FILE = cfg["LOG"]["CLIENT_LOG_FILE"].get<std::string>();
    log_fd = open(LOG_FILE.c_str(), O_CREAT|O_APPEND|O_WRONLY, 0644);
    if (log_fd < 0) { std::cerr<<"로그 파일 열기 실패: "<<LOG_FILE<<"\n"; std::exit(1); }
}

int main() {
    std::signal(SIGINT, handle_sigint);
    load_config("config/config.json");

    SafeQueue<std::pair<double,double>> gps_queue;
    SafeQueue<int>                      cmd_queue;
    SafeQueue<std::string>              log_queue;

    std::thread t_gps(
        gps_thread,
        std::ref(gps_queue),
        std::ref(running)
    );

    std::thread t_comm(
        comm_thread,
        std::ref(gps_queue),
        std::ref(cmd_queue),
        std::ref(log_queue)
    );

    t_gps.join();
    t_comm.join();
    close(log_fd);
    return 0;
}
