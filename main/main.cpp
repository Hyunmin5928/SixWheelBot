// src/main.cpp
#include <thread>
#include <atomic>
#include "SafeQueue.hpp"

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

int main(){
  std::atomic<bool> running{true};

  // 각 모듈 간 데이터 교환용 큐
  SafeQueue<int>                     lidar_queue;    // 예: LiDAR raw
  SafeQueue<int>                     motor_cmd_queue;// 예: 회피 명령 코드
  SafeQueue<std::pair<double,double>> gps_queue;     // 예: {위도,경도}
  SafeQueue<std::string>             vision_route_q; // 예: "REPLAN"
  
  // 쓰레드 생성
  std::thread t_motor(  motor_thread,  std::ref(lidar_queue), std::ref(motor_cmd_queue), std::ref(running));
  std::thread t_gps(    gps_thread,    std::ref(gps_queue),   std::ref(vision_route_q), std::ref(running));
  std::thread t_vision( vision_thread, std::ref(vision_route_q), std::ref(gps_queue),   std::ref(running));
  std::thread t_comm(   comm_thread,   std::ref(gps_queue),   std::ref(motor_cmd_queue), std::ref(running));

  // 필요에 따라 SIGINT 핸들링 → running=false 로 설정

  // 예: 60초 후 자동 종료
  std::this_thread::sleep_for(std::chrono::seconds(60));
  running = false;

  // 종료 대기
  t_motor.join();
  t_gps  .join();
  t_vision.join();
  t_comm .join();

  return 0;
}
