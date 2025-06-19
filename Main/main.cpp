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
#include "Communication/comm_module.h"
#include "GPS/gps_module.h"
#include "GPS/runGPS.h"
#include "logger.h"

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
std::string CLI_LOG_FILE;
int         cli_log_fd;
int         RETRY_LIMIT;
double      ACK_TIMEOUT;
int         sock_fd = -1;

std::atomic<bool> running{true};

// SIGINT 핸들러

void handle_sigint(int) {
    // 디폴트 동작(SIG_DFL)으로 복원
    std::signal(SIGINT, SIG_DFL);
    // 자기 자신에게 다시 SIGINT 발생 → 디폴트(강제종료) 수행
    std::raise(SIGINT);
}
void load_config(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs) { std::cerr<<"설정 파일 열기 실패\n"; std::exit(1); }
    json cfg; ifs >> cfg;
    SERVER_IP    = cfg["SERVER"]["IP"];
    SERVER_PORT  = cfg["SERVER"]["PORT"];
    CLIENT_IP    = cfg["CLIENT"]["IP"];
    CLIENT_PORT  = cfg["CLIENT"]["PORT"];
    CLI_LOG_FILE = cfg["LOG"]["CLIENT_LOG_FILE"];
    RETRY_LIMIT  = cfg["NETWORK"]["RETRY_LIMIT"];
    ACK_TIMEOUT  = cfg["NETWORK"]["ACK_TIMEOUT"];
}

int main(){
    std::signal(SIGINT, handle_sigint);

    load_config("config/config.json");

    // 1) 경로(map) → Route 리스트
    SafeQueue<std::vector<std::tuple<double,double,int>>> map_queue;
    // 2) 현재 위치 (필요시 로깅용)
    SafeQueue<std::pair<double,double>> gps_queue;
    // 3) 방향 코드만
    SafeQueue<int> dir_queue;
    // 4) (선택) 통신 명령용, 로그용 큐
    SafeQueue<int> cmd_queue;
    SafeQueue<std::string> log_queue;

    // 통신 스레드: map_queue, cmd_queue, log_queue
    std::thread t_comm(
        comm_thread,
        std::ref(map_queue),
        std::ref(cmd_queue),
        std::ref(log_queue));

    // GPS 스레드: gps_queue, map_queue, dir_queue
    std::thread t_gps(
        gps_thread,
        std::ref(gps_queue),
        std::ref(map_queue),
        std::ref(dir_queue),
        std::ref(running));

    // runGPS 스레드: dir_queue
    std::thread t_run(
        runGPS,
        std::ref(dir_queue),
        std::ref(gps_queue),
        std::ref(running));

    // running==false 될 때까지 대기
    while (running) std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 종료
    t_comm.join();
    t_gps.join();
    t_run.join();
    close(cli_log_fd);
    return 0;
}
