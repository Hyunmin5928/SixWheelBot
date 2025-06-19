#include "logger.h"
#include <chrono>
#include <ctime>
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <unistd.h>
#include <iostream>

int log_fd = -1;

void init_logger(const std::string& log_file) {
    log_fd = open(log_file.c_str(), O_CREAT | O_APPEND | O_WRONLY, 0644);
    if (log_fd < 0) {
        std::cerr << "ERROR: Cannot open log file: " << log_file << "\n";
    }
}

void close_logger() {
    if (log_fd >= 0) {
        close(log_fd);
        log_fd = -1;
    }
}

void log_msg(const std::string& level, const std::string& msg) {
    if (log_fd < 0) return;

    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S")
        << " [" << level << "] " << msg << "\n";

    write(log_fd, oss.str().c_str(), oss.str().size());
}
