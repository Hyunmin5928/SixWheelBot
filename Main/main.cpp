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
#include <termios.h>

#include <nlohmann/json.hpp>

#include "SafeQueue.hpp"
#include "Communication/comm_module.h"
#include "GPS/gps_module.h"
#include "Motor/motor_module.h"
#include "LiDAR/lidar_module.h"
#include "logger.h"
#include "Vision/vision_module.h"

//-----------------------------------------------------------------------------
// 전역 터미널 원복용 구조체
static struct termios orig_termios;

// 프로그램 종료 시 터미널 설정 복원
void restore_terminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

//-----------------------------------------------------------------------------
// 시그널용 플래그
volatile sig_atomic_t running    = 1;
volatile sig_atomic_t run_lidar  = 1;
volatile sig_atomic_t run_gps    = 1;
volatile sig_atomic_t run_motor  = 1;
volatile sig_atomic_t run_vision = 1;

//-----------------------------------------------------------------------------
// SIGINT 핸들러: 플래그 끄고 터미널 복원, 소켓 닫기
int sock_fd = -1;
void handle_sigint(int) {
    run_lidar   = 0;
    run_gps     = 0;
    run_motor   = 0;
    run_vision  = 0;
    running     = 0;
    // 직렬포트나 소켓이 블로킹 중이면 close()로 깨워주기
    if (sock_fd != -1) {
        close(sock_fd);
        sock_fd = -1;
    }
    restore_terminal();
    // 이후에는 exit() 대신 루프가 깨지고 main에서 return
}

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

using json = nlohmann::json;

// 전역 설정 변수
std::string SERVER_IP;
int         SERVER_PORT;
std::string CLIENT_IP;
int         CLIENT_PORT;
std::string ALLOW_IP;
std::string AI_SERVER_IP;
int         AI_SERVER_PORT;

int         LOG_LEVEL;
std::string CLI_LOG_FILE;
std::string GPS_LOG_FILE;
std::string LIDAR_LOG_FILE;
std::string MOTOR_LOG_FILE;
std::string VISION_LOG_FILE;
std::string COMMAND_LOG_FILE;

int         RETRY_LIMIT;
double      ACK_TIMEOUT;
int         sock_fd = -1;

// 기존 SafeQueue<LaserPoint> lidar_queue 외에…
SafeQueue<std::vector<LaserPoint>> raw_scan_queue;

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
    VISION_LOG_FILE = cfg["LOG"]["VISION_LOG_FILE"];

    // 통신 패킷 관련 설정
    RETRY_LIMIT  = cfg["NETWORK"]["RETRY_LIMIT"];
    ACK_TIMEOUT  = cfg["NETWORK"]["ACK_TIMEOUT"];
    ALLOW_IP     = cfg["NETWORK"]["ALLOW_IP"];
    AI_SERVER_IP   = cfg["NETWORK"]["AI_SERVER_IP"];
    AI_SERVER_PORT = cfg["NETWORK"]["AI_SERVER_PORT"];
}


int main(){
    // 1) 현재 터미널 상태 저장
    tcgetattr(STDIN_FILENO, &orig_termios);
    // 2) 프로그램이 종료될 때 무조건 터미널 복원
    atexit(restore_terminal);

    // 3) SIGINT 핸들러 등록
    std::signal(SIGINT, handle_sigint);

    load_config("../Config/config.json");

    Logger::instance().addFile("comm",   CLI_LOG_FILE,   static_cast<LogLevel>(LOG_LEVEL));
    Logger::instance().addFile("gps",    GPS_LOG_FILE,   static_cast<LogLevel>(LOG_LEVEL));
    Logger::instance().addFile("lidar",  LIDAR_LOG_FILE, static_cast<LogLevel>(LOG_LEVEL));
    Logger::instance().addFile("motor",  MOTOR_LOG_FILE, static_cast<LogLevel>(LOG_LEVEL));
    Logger::instance().addFile("vision", VISION_LOG_FILE,static_cast<LogLevel>(LOG_LEVEL));

    // 1) 경로(map) → Route 리스트
    SafeQueue<std::vector<std::tuple<double,double,int>>> map_queue;
    // 2) 현재 위치 (필요시 로깅용)
    SafeQueue<std::pair<double,double>> gps_queue;
    // 3) 방향 코드만
    SafeQueue<float> dir_queue_g;
    SafeQueue<float> dir_queue_v;
    // 4) 통신 명령용, 로그용 큐
    SafeQueue<int> cmd_queue;
    SafeQueue<std::string> log_queue;
    // 5) LiDAR 센서 큐
    SafeQueue<LaserPoint> lidar_queue;
    // 6) GPS 센서 -> 배달지 도착 및 복귀 장소 도착 구분 플래그 큐
    SafeQueue<bool> m_stop_queue;
    m_stop_queue.Produce(true);
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
        0, 
        navigation_thread,
        std::ref(map_queue),
        std::ref(dir_queue_g),
        std::ref(m_stop_queue),
        std::ref(cmd_queue)
    );

    // 5) LiDAR 스캔 프로듀서 -> 2
    std::thread t_lidar_prod = start_thread_with_affinity(
        2, 
        lidar_producer,
        std::ref(lidar_queue)
    );

    // 6) 모터 스레드 -> 3
    std::thread t_motor = start_thread_with_affinity(
        3,
        motor_thread,
        "/dev/ttyUSB0",
        115200,
        std::ref(dir_queue_g),
        std::ref(dir_queue_v),
        std::ref(lidar_queue),
        std::ref(m_stop_queue)
    );

    // 7) 비전 스레드 추가 -> 2
    std::thread t_vision = start_thread_with_affinity(
        1,
        vision_thread,
        std::ref(dir_queue_v)           
    );

    // 메인 루프
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 종료 대기
    t_comm.join();
    t_gps_reader.join();
    t_gps_sender.join();
    t_nav.join();
    t_lidar_prod.join();
    t_motor.join();
    t_vision.join();  

    return 0;
}