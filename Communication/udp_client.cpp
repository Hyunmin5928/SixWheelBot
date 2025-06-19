#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <thread>
#include <csignal>
#include <cmath>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/*
    로그 함수 사용 법
    // 로그 초기화: 파일 경로, 최소 출력 레벨 지정
    util::Logger::instance().init("app.log", util::LogLevel::Debug);

    util::Logger::instance().info("프로그램 시작");
    util::Logger::instance().debug("디버그 메시지");
    util::Logger::instance().warn("경고 메시지");
    util::Logger::instance().error("오류 메시지");
*/

// 전역 설정 변수
std::string SERVER_IP;
int         SERVER_PORT;
std::string CLIENT_IP;
int         CLIENT_PORT;
std::string ALLOW_IP;
double      ACK_TIMEOUT;
int         RETRY_LIMIT;
std::string LOG_FILE;

const char* PID_FILE = "/var/run/udp_client.pid";
// int log_fd;

// **전역 소켓 디스크립터**
int sock_fd = -1;

// 함수 프로토타입 (별도 모듈에서 구현)
std::vector<double> get_current_gps();
void                send_to_servo_module(const std::string& cmd);

// 로그 함수
// void log_msg(const std::string& level, const std::string& msg) {
//     auto now = std::chrono::system_clock::to_time_t(
//                    std::chrono::system_clock::now());
//     std::ostringstream oss;
//     oss << std::put_time(std::localtime(&now),
//                          "%Y-%m-%d %H:%M:%S")
//         << " [" << level << "] " << msg << "\n";
//     write(log_fd, oss.str().c_str(), oss.str().size());
// }

// 설정 파일 로드
void load_config(const std::string& path) {
    std::ifstream ifs(path);
    json cfg; ifs >> cfg;
    SERVER_IP   = cfg["SERVER"]["IP"].get<std::string>();
    SERVER_PORT = cfg["SERVER"]["PORT"].get<int>();
    CLIENT_IP   = cfg["CLIENT"]["IP"].get<std::string>();
    CLIENT_PORT = cfg["CLIENT"]["PORT"].get<int>();
    ALLOW_IP    = cfg["NETWORK"]["ALLOW_IP"].get<std::string>();
    ACK_TIMEOUT = cfg["NETWORK"]["ACK_TIMEOUT"].get<double>();
    RETRY_LIMIT = cfg["NETWORK"]["RETRY_LIMIT"].get<int>();
    LOG_FILE    = cfg["LOG"]["CLIENT_LOG_FILE"].get<std::string>();
}

// 종료 시그널(SIGTERM) 핸들러
void cleanup_and_exit(int) {
    if (sock_fd >= 0) close(sock_fd);
    unlink(PID_FILE);
    exit(0);
}

// 데몬라이즈
void daemonize() {
    pid_t pid = fork();
    if (pid > 0) exit(0);
    setsid();
    pid = fork();
    if (pid > 0) exit(0);
    umask(0);

    // PID 파일 작성
    std::ofstream pidf(PID_FILE);
    pidf << getpid();
    pidf.close();

    // SIGTERM 받으면 cleanup_and_exit 호출
    signal(SIGTERM, cleanup_and_exit);
}

// ACK 대기 함수 (기존)
void send_and_wait_ack(int sock, const std::string& data,
                       const sockaddr_in& srv_addr,
                       const std::string& ack_str,
                       std::map<int,std::string>& cache,
                       int key)
{
    for (int i = 0; i < RETRY_LIMIT; ++i) {
        sendto(sock, data.data(), data.size(), 0,
               (sockaddr*)&srv_addr, sizeof(srv_addr));

        char buf[1024];
        struct timeval tv{
            (int)ACK_TIMEOUT,
            int((ACK_TIMEOUT - int(ACK_TIMEOUT))*1e6)
        };
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        ssize_t n = recvfrom(sock, buf, sizeof(buf)-1, 0, nullptr, nullptr);
        if (n > 0) {
            buf[n] = '\0';
            if (std::string(buf) == ack_str) {
                cache.erase(key);
                return;
            }
        }
        log_msg("WARNING", ack_str + " retry " + std::to_string(i+1));
    }
}

// 두 GPS 좌표 사이 거리 계산 (m 단위)
double distance_m(const std::vector<double>& a,
                  const std::vector<double>& b)
{
    double dx = (a[0]-b[0]) * 111000.0;
    double dy = (a[1]-b[1]) * 111000.0;
    return std::sqrt(dx*dx + dy*dy);
}

// 클라이언트 메인 루프
void client_loop() {
    // 소켓 생성 및 전역 변수에 저장
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in cli_addr{}, srv_addr{};
    cli_addr.sin_family      = AF_INET;
    cli_addr.sin_port        = htons(CLIENT_PORT);
    cli_addr.sin_addr.s_addr = (CLIENT_IP=="0.0.0.0"
                                ? INADDR_ANY
                                : inet_addr(CLIENT_IP.c_str()));
    bind(sock_fd, (sockaddr*)&cli_addr, sizeof(cli_addr));

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port   = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP.c_str(), &srv_addr.sin_addr);

    // 1) map 수신 & ACK_MAP
    char buf[65536];
    std::vector<std::vector<double>> route;
    while (true) {
        ssize_t len = recvfrom(sock_fd, buf, sizeof(buf)-1, 0,
                               nullptr, nullptr);
        if (len <= 0) continue;
        buf[len] = '\0';
        std::string s(buf);
        if (s.rfind("{",0)!=0) continue;
        auto pkt = json::parse(s);
        if (pkt["type"] == "map") {
            route = pkt["route"].get<decltype(route)>();
            sendto(sock_fd, "ACK_MAP:0", 9, 0,
                   (sockaddr*)&cli_addr, sizeof(cli_addr));
            log_msg("INFO", "Map received: " +
                     std::to_string(route.size()) + " points");
            break;
        }
    }

    // 상태 관리
    enum State{ MOVING, PAUSED, UNLOCKED, RETURNING };
    State state = MOVING;
    auto destination = route.back();
    int packet_num = 0;
    std::map<int,std::string> cache;

    // 2) 메인 송수신 루프 (절대 소켓 닫지 않음)
    while (true) {
        // 로그 전송
        auto current_gps = get_current_gps();
        json log_pkt = {
            {"packet_number", packet_num},
            {"type",           "log"},
            {"robot_state",    state==MOVING ? "moving"
                              : state==RETURNING ? "returning"
                              : "paused"},
            {"gps",            current_gps},
            {"timestamp",      time(nullptr)}
        };
        std::string data = log_pkt.dump();
        cache[packet_num] = data;
        send_and_wait_ack(sock_fd, data, srv_addr,
                          "ACK_LOG:"+std::to_string(packet_num),
                          cache, packet_num);
        ++packet_num;

        // 서버로부터 명령(non-blocking)
        struct timeval tv{0,0};
        setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        ssize_t n;
        while ((n = recvfrom(sock_fd, buf, sizeof(buf)-1,
                             0, nullptr, nullptr)) > 0)
        {
            buf[n] = '\0';
            auto cmd = json::parse(buf);
            if (cmd["type"] == "cmd") {
                std::string action = cmd["action"];
                if (action == "pause") {
                    state = PAUSED;
                    log_msg("INFO", "Paused at destination");
                }
                else if (action == "unlock") {
                    state = UNLOCKED;
                    send_to_servo_module("unlock");
                    log_msg("INFO", "Unlock command forwarded");
                }
                else if (action == "return") {
                    state = RETURNING;
                    log_msg("INFO", "Return command received");
                }
            }
        }

        // 도착/복귀 완료 체크
        if ((state==MOVING || state==RETURNING) &&
            distance_m(current_gps,
                       state==MOVING?destination:route.front())
            <= 0.5)
        {
            log_msg("INFO", "Within 0.5m of target, waiting for pause");
            // 블록킹 모드로 pause 명령 대기
            while (true) {
                ssize_t m = recvfrom(sock_fd, buf,
                                     sizeof(buf)-1, 0,
                                     nullptr, nullptr);
                if (m <= 0) continue;
                buf[m] = '\0';
                auto cmd = json::parse(buf);
                if (cmd["type"]=="cmd" &&
                    cmd["action"]=="pause")
                {
                    log_msg("INFO", "Final pause confirmed");
                    break;
                }
            }
        }

        std::this_thread::sleep_for(
            std::chrono::seconds(2));
    }

    // **close(sock_fd); 절대 여기서 호출하지 않습니다.**
}

int main(int argc, char* argv[]) {
    load_config("config/config.json");
    log_fd = open(LOG_FILE.c_str(),
                  O_CREAT|O_APPEND|O_WRONLY, 0644);

    if (argc != 2) {
        std::cout << "Usage: " << argv[0]
                  << " [start|stop|restart|status]\n";
        return 1;
    }
    std::string cmd = argv[1];

    if (cmd == "start") {
        log_msg("INFO","Client Daemon start");
        daemonize();
        client_loop();
    }
    else if (cmd == "stop") {
        log_msg("INFO","Client Daemon stop");
        std::ifstream pidf(PID_FILE);
        pid_t pid; 
        if (pidf >> pid) kill(pid, SIGTERM);
    }
    else if (cmd == "restart") {
        log_msg("INFO","Client Daemon restart");
        std::ifstream pidf(PID_FILE);
        pid_t pid; 
        if (pidf >> pid) kill(pid, SIGTERM);
        std::this_thread::sleep_for(
            std::chrono::seconds(1));
        daemonize();
        client_loop();
    }
    else if (cmd == "status") {
        if (access(PID_FILE, F_OK) == 0)
            std::cout << "UDP Client Daemon Running\n";
        else
            std::cout << "Stopped\n";
    }
    else {
        std::cout << "Unknown command\n"
                  << "Usage: " << argv[0]
                  << " [start|stop|restart|status]\n";
    }

    close(log_fd);
    return 0;
}
