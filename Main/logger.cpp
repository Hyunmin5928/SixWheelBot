#include "logger.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstring>

namespace util {

// helper: 문자열 레벨 → LogLevel
static LogLevel stringToLevel(const char* s) {
    if (!std::strcmp(s,"DEBUG")) return LogLevel::Debug;
    if (!std::strcmp(s,"INFO"))  return LogLevel::Info;
    if (!std::strcmp(s,"WARN"))  return LogLevel::Warn;
    if (!std::strcmp(s,"ERROR")) return LogLevel::Error;
    return LogLevel::Info;
}

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

Logger::~Logger() {
    for (auto& [_, e] : files_) if (e.ofs.is_open())
        e.ofs.close();
}

void Logger::addFile(const std::string& tag,
                     const std::string& filepath,
                     LogLevel lvl) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto& entry = files_[tag];
    if (entry.ofs.is_open()) entry.ofs.close();
    entry.ofs.open(filepath, std::ios::app);
    entry.level = lvl;
}

void Logger::debug(const std::string& tag, const std::string& msg) {
    log(tag, "DEBUG", msg);
}
void Logger::info (const std::string& tag, const std::string& msg) {
    log(tag, "INFO",  msg);
}
void Logger::warn (const std::string& tag, const std::string& msg) {
    log(tag, "WARN",  msg);
}
void Logger::error(const std::string& tag, const std::string& msg) {
    log(tag, "ERROR", msg);
}

void Logger::log(const std::string& tag,
                 const char* levelStr,
                 const std::string& msg) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = files_.find(tag);
    if (it == files_.end()) return;
    auto& entry = it->second;
    LogLevel msgLevel = stringToLevel(levelStr);
    if (msgLevel < entry.level) return;

    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                   now.time_since_epoch()).count() % 1000;

    std::tm tm_buf;
    localtime_r(&t, &tm_buf);

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << ms
        << " [" << levelStr << "] " << msg << '\n';

    entry.ofs << oss.str();
    entry.ofs.flush();
}

} // namespace util
