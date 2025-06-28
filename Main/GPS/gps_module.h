#pragma once
#include <atomic>
#include <vector>
#include <sstream>
#include <tuple>
#include "../SafeQueue.hpp"
#include "../logger.h"
#include "lib/GPS_parse.h"     // sGPS, GPS 클래스
// using Route = std::vector<std::tuple<double,double,int>>;
// sudo ln -sf /dev/ttyAMA0 /dev/serial0

extern std::string GPS_LOG_FILE;
extern int         LOG_LEVEL;
extern std::atomic<bool> running;
extern std::atomic<bool> run_gps;
extern std::atomic<bool> run_motor;

using util::Logger;
using util::LogLevel;
using GpsPos   = std::pair<double,double>;          // {lat, lon}
using Waypoint = std::tuple<double,double,int>;      // {lat, lon, dirCode}

/**
 * navigation_thread:
 *   - map_q에서 전체 경로(waypoint 리스트)를 한 번 수신
 *   - 내부에서 GPS 센서를 직접 읽어 현재 위치 얻음
 *   - 현재 위치와 waypoint를 비교하여 dirCode를 m_cmd_q에 푸시
 *   - running이 false가 되면 정지 명령 후 종료
 */
void navigation_thread(
    SafeQueue<std::vector<Waypoint>>&   map_q,
    SafeQueue<float>&                   dir_queue,
    SafeQueue<bool>&                    m_stop_queue
);

// GPS 읽기 전용 스레드 함수
void gps_reader_thread(
    SafeQueue<GpsPos>& gps_q
);

// void gps_thread(
//     SafeQueue<std::pair<double,double>>& gps_q,
//     SafeQueue<Route>&                    map_q,
//     SafeQueue<int>&                      dir_q,
//     std::atomic<bool>&                   running);