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
#include <nlohmann/json.hpp>
#include "SafeQueue.hpp"

using json = nlohmann::json;

// 전역 설정 변수
std::string SERVER_IP;
int         SERVER_PORT;
std::string CLIENT_IP;
int         CLIENT_PORT;
std::string ALLOW_IP;
double      ACK_TIMEOUT;
int         RETRY_LIMIT;
std::string LOG_FILE;

int log_fd;

// 전역 종료 플래그
std::atomic<bool> running{true};

// 모듈 헤더 (각 src/*.cpp 에 정의)
void motor_thread(SafeQueue<int>& lidar_q,
                  SafeQueue<int>& cmd_q,
                  std::atomic<bool>& running);

void gps_thread(SafeQueue<std::pair<double,double>>& gps_q,
                SafeQueue<std::string>& route_q,
                std::atomic<bool>& running);

void vision_thread(SafeQueue<std::string>& route_q,
                   SafeQueue<std::pair<double,double>>& gps_q,
                   std::atomic<bool>& running);

void comm_thread(SafeQueue<std::pair<double,double>>& gps_q,
                 SafeQueue<int>& cmd_q,
                 std::atomic<bool>& running);

// SIGINT 핸들러: Ctrl+C로 안전 종료
void handle_sigint(int) {
  running = false;
}

// 설정 파일 로드
void load_config(const std::string& path) {
  std::ifstream ifs(path);
  if (!ifs) {
    std::cerr<<"설정 파일 열기 실패: "<<path<<"\n";
    std::exit(1);
  }
  json cfg; 
  ifs >> cfg;
  SERVER_IP   = cfg["SERVER"]["IP"].get<std::string>();
  SERVER_PORT = cfg["SERVER"]["PORT"].get<int>();
  CLIENT_IP   = cfg["CLIENT"]["IP"].get<std::string>();
  CLIENT_PORT = cfg["CLIENT"]["PORT"].get<int>();
  ALLOW_IP    = cfg["NETWORK"]["ALLOW_IP"].get<std::string>();
  ACK_TIMEOUT = cfg["NETWORK"]["ACK_TIMEOUT"].get<double>();
  RETRY_LIMIT = cfg["NETWORK"]["RETRY_LIMIT"].get<int>();
  LOG_FILE    = cfg["LOG"]["CLIENT_LOG_FILE"].get<std::string>();
}

// 로그 함수
void log_msg(const std::string& level, const std::string& msg) {
  auto now = std::chrono::system_clock::to_time_t(
                  std::chrono::system_clock::now());
  std::ostringstream oss;
  oss << std::put_time(std::localtime(&now),
                        "%Y-%m-%d %H:%M:%S")
      << " [" << level << "] " << msg << "\n";
  write(log_fd, oss.str().c_str(), oss.str().size());
}


int main(){
  std::atomic<bool> running{true};
  load_config("config/config.json");
    log_fd = open(LOG_FILE.c_str(),
                  O_CREAT|O_APPEND|O_WRONLY, 0644);
  if (log_fd < 0) {
    std::cerr<<"로그 파일 열기 실패: "<<LOG_FILE<<"\n";
    return 1;
  }
  // 각 모듈 간 데이터 교환용 큐
  SafeQueue<int>                      lidar_queue;    // 예: LiDAR raw
  SafeQueue<int>                      motor_cmd_queue;// 예: 회피 명령 코드
  SafeQueue<std::pair<double,double>> gps_queue;     // 예: {위도,경도}
  SafeQueue<std::string>              vision_route_q; // 예: "REPLAN"
  
  // 쓰레드 생성
  std::thread t_motor(  motor_thread,  std::ref(lidar_queue), std::ref(motor_cmd_queue), std::ref(running));
  std::thread t_gps(    gps_thread,    std::ref(gps_queue),   std::ref(vision_route_q), std::ref(running));
  std::thread t_vision( vision_thread, std::ref(vision_route_q), std::ref(gps_queue),   std::ref(running));
  std::thread t_comm(   comm_thread,   std::ref(gps_queue),   std::ref(motor_cmd_queue), std::ref(running));

  // 필요에 따라 SIGINT 핸들링 → running=false 로 설정

  // 예: 60초 후 자동 종료
  std::this_thread::sleep_for(std::chrono::seconds(60));
  while (running) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  // 종료 대기
  t_motor.join();
  t_gps  .join();
  t_vision.join();
  t_comm .join();

  close(log_fd);

  return 0;
}


모터함수

1. 전진
2. 후진
3. 회전(제자리 회전) -> 회전 각도(수치 pwm 차이값으로) -> 시도해보면서 pwm넣고 이때는 어느정도 도네? 시간(고정) -> 