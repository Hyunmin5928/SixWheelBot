#include "logger.h"
#include <chrono>
#include <ctime>
#include <iomanip>

namespace util {

// 싱글톤 인스턴스
Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

Logger::~Logger() {
    if (_ofs.is_open()) _ofs.close();
}

void Logger::init(const std::string& logFilePath, LogLevel level) {
    std::lock_guard<std::mutex> lock(_mtx);
    if (_ofs.is_open()) _ofs.close();
    _ofs.open(logFilePath, std::ios::app);
    _level = level;
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(_mtx);
    _level = level;
}

void Logger::trace(const std::string& msg) { log(LogLevel::Trace, msg); }
void Logger::debug(const std::string& msg) { log(LogLevel::Debug, msg); }
void Logger::info (const std::string& msg) { log(LogLevel::Info,  msg); }
void Logger::warn (const std::string& msg) { log(LogLevel::Warn,  msg); }
void Logger::error(const std::string& msg) { log(LogLevel::Error, msg); }
void Logger::fatal(const std::string& msg) { log(LogLevel::Fatal, msg); }

void Logger::log(LogLevel msgLevel, const std::string& msg) {
    if (msgLevel < _level) return;

    // 타임스탬프 생성
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                   now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count()
        << " [" << levelToString(msgLevel) << "] " << msg << '\n';

    std::lock_guard<std::mutex> lock(_mtx);
    if (_ofs.is_open()) {
        _ofs << oss.str();
        _ofs.flush();
    }
}

const char* Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
    }
    return "UNKNOWN";
}

} // namespace util
