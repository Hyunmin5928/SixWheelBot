#pragma once
#include "../SafeQueue.hpp"
#include "../logger.h"
#include <opencv2/opencv.hpp>            // OpenCV 핵심 헤더 (영상 캡처·인코딩)
#include <arpa/inet.h>                  // htonl, htons, inet_pton 등 IP/포트 변환
#include <sys/socket.h>                 // socket, connect, send, recv
#include <unistd.h>                     // close
#include <netinet/in.h>                 // sockaddr_in 구조체
#include <atomic>                       // std::atomic<bool>
#include <chrono>                       // 시간 측정·sleep
#include <cstring>                      // strerror
#include <iostream>                     // 콘솔 입출력
#include <stdexcept>                    // 예외 처리
#include <string>                       // std::string
#include <thread>                       // std::thread, this_thread::sleep_for
#include <vector>                       // std::vector<uchar>

extern std::string          LIDAR_LOG_FILE;
extern int                  LOG_LEVEL;
extern std::atomic<bool>    running;
extern std::atomic<bool>    run_vision;
extern std::string          AI_SERVER_IP
extern int                  AI_SERVER_PORT;

using util::Logger;                             
using util::LogLevel;

void vision_thread(SafeQueue<float>& dir_queue);
static void recvall(int sock, void* buf, size_t len);
static void sendall(int sock, const void* buf, size_t len);