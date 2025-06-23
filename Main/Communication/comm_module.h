#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <tuple>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <atomic>
#include <map>
#include <thread>
#include <cmath>
#include <chrono>
#include "../SafeQueue.hpp"
#include "../logger.h"
using json = nlohmann::json;
using util::Logger;
using util::LogLevel;

extern std::atomic<bool> running;
extern std::atomic<bool> run_imu;
extern std::atomic<bool> run_lidar;
extern std::atomic<bool> run_gps;
extern int         sock_fd;
extern std::string SERVER_IP;
extern int         SERVER_PORT;
extern std::string CLIENT_IP;
extern int         CLIENT_PORT;
extern int         LOG_LEVEL;
extern std::string CLI_LOG_FILE;
extern int         RETRY_LIMIT;
extern double      ACK_TIMEOUT;

// map data 수신 후 map_queue에 전달 only
void comm_thread(
    SafeQueue<std::vector<std::tuple<double,double,int>>>& map_q,
    SafeQueue<int>&                                        cmd_q,
    SafeQueue<std::string>&                                log_q);

void log_sender_thread(SafeQueue<std::string>& log_q);

void cmd_receiver_thread(SafeQueue<int>& cmd_q);

void gps_sender_thread(SafeQueue<std::pair<double,double>>& gps_q);

void send_and_wait_ack(int sock,const std::string& data,const sockaddr_in& srv_addr,const std::string& ack_str,std::map<int,std::string>& cache,int key);
