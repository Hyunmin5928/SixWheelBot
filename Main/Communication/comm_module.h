#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <map>
#include <thread>
#include <nlohmann/json.hpp>
#include <atomic>
#include "../SafeQueue.hpp"

extern std::atomic<bool> running;
extern int         sock_fd;
extern std::string SERVER_IP;
extern int         SERVER_PORT;
extern std::string CLIENT_IP;
extern int         CLIENT_PORT;
extern int         log_fd;
extern int         RETRY_LIMIT;
extern double      ACK_TIMEOUT;

void comm_thread(
    SafeQueue<std::pair<double,double>>& gps_q,
    SafeQueue<int>&                      cmd_q,
    SafeQueue<std::string>&              log_q);

void log_sender_thread(
    SafeQueue<std::string>& log_q);

void cmd_receiver_thread(
    SafeQueue<int>& cmd_q);
