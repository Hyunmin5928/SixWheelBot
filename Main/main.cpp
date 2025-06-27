#include <thread>
#include <atomic>
#include <csignal>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <pthread.h>
#include <sched.h>

#include <nlohmann/json.hpp>

#include "SafeQueue.hpp"
#include "Communication/comm_module.h"
#include "GPS/gps_module.h"
#include "Motor/motor_module.h"
#include "LiDAR/lidar_module.h"
#include "LiDAR/Lidar.h"
#include "logger.h"
#include "Vision/vision_module.h"

// main.cpp 맨 위, includes 아래에 추가
template<typename F, typename... Args>
std::thread start_thread_with_affinity(int core_id, F&& f, Args&&... args) {
  return std::thread([=]() {
    // 1) 자신(POSIX 스레드)의 affinity 설정
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) != 0) {
      perror("pthread_setaffinity_np");
    }
    // 2) 실제 스레드 함수 호출
    std::invoke(f, args...);
  });
}

using util::Logger;
using util::LogLevel;
/*
    로그 함수 사용 법
    // 로그 초기화: 파일 경로, 최소 출력 레벨 지정
    util::Logger::instance().init("app.log", util::LogLevel::Debug);

    util::Logger::instance().info("프로그램 시작");
    util::Logger::instance().debug("디버그 메시지");
    util::Logger::instance().warn("경고 메시지");
    util::Logger::instance().error("오류 메시지");
*/

using json = nlohmann::json;

// 전역 설정 변수
std::string SERVER_IP;
int         SERVER_PORT;
std::string CLIENT_IP;
int         CLIENT_PORT;
std::string ALLOW_IP;

int         LOG_LEVEL;
std::string CLI_LOG_FILE;
std::string GPS_LOG_FILE;
std::string LIDAR_LOG_FILE;
std::string MOTOR_LOG_FILE;
std::string IMU_LOG_FILE;
std::string VISION_LOG_FILE;
std::string COMMAND_LOG_FILE;

int         RETRY_LIMIT;
double      ACK_TIMEOUT;
int         sock_fd = -1;

std::atomic<bool> running{true};
// std::atomic<bool> run_imu{false};
std::atomic<bool> run_lidar{false};
std::atomic<bool> run_gps{false};
std::atomic<bool> run_motor{false};
std::atomic<bool> run_command{false};

// 기존 SafeQueue<LaserPoint> lidar_queue 외에…
SafeQueue<std::vector<LaserPoint>> raw_scan_queue;

static constexpr const char cmd_stop1 [] = "stop\n";

// SIGINT 핸들러: Ctrl+C 시 running 플래그만 false 로 전환
void handle_sigint(int) {
    run_lidar.store(false);
    run_gps.store(false);
    // run_imu.store(false);
    running.store(false);
    run_motor.store(false);
    run_command.store(false);
}

void load_config(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs) { std::cerr<<"설정 파일 열기 실패\n"; std::exit(1); }
    json cfg; ifs >> cfg;
    SERVER_IP    = cfg["SERVER"]["IP"];
    SERVER_PORT  = cfg["SERVER"]["PORT"];
    CLIENT_IP    = cfg["CLIENT"]["IP"];
    CLIENT_PORT  = cfg["CLIENT"]["PORT"];
    
    // log 파일 경로 지정
    LOG_LEVEL       = cfg["LOG"]["LOG_LEVEL"];
    CLI_LOG_FILE    = cfg["LOG"]["CLIENT_LOG_FILE"];
    GPS_LOG_FILE    = cfg["LOG"]["GPS_LOG_FILE"];
    LIDAR_LOG_FILE  = cfg["LOG"]["LIDAR_LOG_FILE"];
    MOTOR_LOG_FILE  = cfg["LOG"]["MOTOR_LOG_FILE"];
    // IMU_LOG_FILE    = cfg["LOG"]["IMU_LOG_FILE"];
    VISION_LOG_FILE = cfg["LOG"]["VISION_LOG_FILE"];

    // 통신 패킷 관련 설정
    RETRY_LIMIT  = cfg["NETWORK"]["RETRY_LIMIT"];
    ACK_TIMEOUT  = cfg["NETWORK"]["ACK_TIMEOUT"];
    ALLOW_IP     = cfg["NETWORK"]["ALLOW_IP"];
}


int main(){
    std::signal(SIGINT, handle_sigint);

    load_config("config/config.json");

    Logger::instance().addFile("comm",   CLI_LOG_FILE,   static_cast<LogLevel>(LOG_LEVEL));
    Logger::instance().addFile("gps",    GPS_LOG_FILE,   static_cast<LogLevel>(LOG_LEVEL));
    Logger::instance().addFile("lidar",  LIDAR_LOG_FILE, static_cast<LogLevel>(LOG_LEVEL));
    Logger::instance().addFile("motor",  MOTOR_LOG_FILE, static_cast<LogLevel>(LOG_LEVEL));
    // Logger::instance().addFile("imu",    IMU_LOG_FILE,   static_cast<LogLevel>(LOG_LEVEL));
    // Logger::instance().addFile("vision", VISION_LOG_FILE,static_cast<LogLevel>(LOG_LEVEL));

    // 1) 경로(map) → Route 리스트
    SafeQueue<std::vector<std::tuple<double,double,int>>> map_queue;
    // 2) 현재 위치 (필요시 로깅용)
    SafeQueue<std::pair<double,double>> gps_queue;
    // 3) 방향 코드만
    SafeQueue<float> dir_queue;
    // 4) (선택) 통신 명령용, 로그용 큐
    SafeQueue<int> cmd_queue;
    SafeQueue<std::string> log_queue;
    //SafeQueue<int> dir_queue;
    // SafeQueue<IMU::Command> imu_cmd_queue;

    // 5) LiDAR 센서 큐
    SafeQueue<LaserPoint> lidar_queue;
    
    // 1) 통신 스레드 -> 1
    std::thread t_comm =  start_thread_with_affinity(
        1,
        comm_thread,
        std::ref(map_queue),
        std::ref(cmd_queue),
        std::ref(log_queue)
    );

    // 2) GPS 읽기 스레드 -> 0
    std::thread t_gps_reader =  start_thread_with_affinity(
        0,
        gps_reader_thread,
        std::ref(gps_queue)
    );

    // 3) GPS 송신 스레드 -> 0 
    std::thread t_gps_sender = start_thread_with_affinity(
        0, 
        gps_sender_thread,
        std::ref(gps_queue)
    );

    // 4) 네비게이션 스레드 -> 1
    std::thread t_nav = start_thread_with_affinity(
        1, 
        navigation_thread,
        std::ref(map_queue),
        std::ref(dir_queue)
    );

    // 5) IMU 스레드 -> 1
    // std::thread t_imu = start_thread_with_affinity(
    //     1,
    //     imureader_thread,
    //     "/dev/ttyUSB0",
    //     115200u,
    //     std::ref(imu_queue)
    // );
    

    // 7) LiDAR 스캔 프로듀서 -> 2
    std::thread t_lidar_prod = start_thread_with_affinity(
        2, 
        lidar_producer
    );

    // 8) LiDAR near-point 컨슈머 -> 3
    std::thread t_lidar_cons = start_thread_with_affinity(
        3,
        lidar_consumer,
        std::ref(raw_scan_queue),
        std::ref(lidar_queue)
    );

    // 9) 모터 스레드 -> 0
    std::thread t_motor = start_thread_with_affinity(
        0,
        motor_thread,
        "/dev/ttyUSB0",
        115200,
        std::ref(dir_queue),
        std::ref(lidar_queue)
    );

    //비전 스레드 추가
    std::thread t_vision = start_thread_with_affinity(
        0,
        vision_thread,
        std::ref(dir_queue)           
    );


    // 메인 루프
    while (running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 종료 대기
    t_comm.join();
    t_gps_reader.join();
    t_gps_sender.join();
    t_nav.join();
    t_lidar_prod.join();
    t_lidar_cons.join();
    t_motor.join();
    t_vision.join();  

    return 0;
}