#pragma once
#include <string>
#include <mutex>
#include <unordered_map>
#include <fstream>

namespace util {

enum class LogLevel { Debug, Info, Warn, Error };

class Logger {
public:
    static Logger& instance();

    void addFile(const std::string& tag,
                 const std::string& filepath,
                 LogLevel lvl = LogLevel::Info);

    void info (const std::string& tag, const std::string& msg);
    void debug(const std::string& tag, const std::string& msg);
    void warn (const std::string& tag, const std::string& msg);
    void error(const std::string& tag, const std::string& msg);

private:
    struct Entry { std::ofstream ofs; LogLevel level; };

    std::mutex mtx_;
    std::unordered_map<std::string, Entry> files_;

    void log(const std::string& tag,
             const char* levelStr,
             const std::string& msg);

    Logger() = default;
    ~Logger();
};

} // namespace util
