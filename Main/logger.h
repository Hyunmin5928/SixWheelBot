// Logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <string>

void init_logger(const std::string& log_file);
void close_logger();
void log_msg(const std::string& level, const std::string& msg);

#endif // LOGGER_H
