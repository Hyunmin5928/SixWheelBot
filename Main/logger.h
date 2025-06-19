#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>

/* 사용방법
using util::Logger;
using util::LogLevel;
상단부에 선언

Logger::instance().init("CLI_LOG_FILE 경로", LogLevel::Debug);
또는
Logger::instance().init(CLI_LOG_FILE, static_cast<util::LogLevel>(LOG_LEVEL)); -> LOG_LEVEL을 config파일에서 불러올 때, INT형이므로 convert 필요

이후

Logger::instance().info("Map data received and ACK sent");
Logger::instance().warn(ack_str + " retry " + std::to_string(i+1));

이런식으로 활용 가능!
*/

namespace util {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal
};

class Logger {
public:
    // 싱글톤 인스턴스 반환
    static Logger& instance();

    // 초기화: 로그 파일 경로와 최소 로그 레벨 지정
    void init(const std::string& logFilePath, LogLevel level);

    // 로그 레벨 변경
    void setLevel(LogLevel level);

    // 메시지 기록
    void trace(const std::string& msg);
    void debug(const std::string& msg);
    void info (const std::string& msg);
    void warn (const std::string& msg);
    void error(const std::string& msg);
    void fatal(const std::string& msg);

private:
    Logger() = default;
    ~Logger();

    // 실제 기록 함수
    void log(LogLevel msgLevel, const std::string& msg);

    std::ofstream _ofs;
    LogLevel      _level = LogLevel::Info;
    std::mutex    _mtx;

    // 레벨을 문자열로 변환
    static const char* levelToString(LogLevel level);
};

} // namespace util

#endif // LOGGER_H
