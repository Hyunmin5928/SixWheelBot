// src/comm_module.cpp
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <map>
#include <chrono>
#include <thread>
#include <nlohmann/json.hpp>
#include "SafeQueue.hpp"

// udp_client.cpp 에 정의된 함수/변수들
extern int        sock_fd;
extern std::string SERVER_IP;
extern int         SERVER_PORT;
extern std::string CLIENT_IP;
extern int         CLIENT_PORT;
extern double      ACK_TIMEOUT;
extern int         RETRY_LIMIT;
extern int         log_fd;

void    send_and_wait_ack(int sock, const std::string& data,
                          const sockaddr_in& srv_addr,
                          const std::string& ack_str,
                          std::map<int,std::string>& cache,
                          int key);

void    log_msg(const std::string& level, const std::string& msg);
std::vector<double> get_current_gps();

using json = nlohmann::json;

void comm_thread(SafeQueue<std::pair<double,double>>& gps_q,
                 SafeQueue<int>& cmd_q,
                 std::atomic<bool>& running)
{
    // 1) 소켓 생성 & 바인드
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in cli{}, srv{};
    cli.sin_family      = AF_INET;
    cli.sin_port        = htons(CLIENT_PORT);
    cli.sin_addr.s_addr = (CLIENT_IP=="0.0.0.0"
                           ? INADDR_ANY
                           : inet_addr(CLIENT_IP.c_str()));
    bind(sock_fd, (sockaddr*)&cli, sizeof(cli));

    srv.sin_family = AF_INET;
    srv.sin_port   = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP.c_str(), &srv.sin_addr);

    // 논블로킹으로 서버→명령 수신
    fcntl(sock_fd, F_SETFL, O_NONBLOCK);

    // 2) 로그 전송용 캐시 & 패킷 번호
    std::map<int,std::string> cache;
    int packet_num = 0;

    char buf[65536];
    while (running) {
        // --- 2-1. GPS 데이터 수집 & 로그 전송 ---
        std::pair<double,double> pos;
        if (gps_q.Consume(pos)) {
            // JSON 패킷 생성
            json log_pkt = {
                {"packet_number", packet_num},
                {"type",           "log"},
                {"robot_state",    "moving"},
                {"gps",            { pos.first, pos.second }},
                {"timestamp",      std::time(nullptr)}
            };
            std::string data = log_pkt.dump();
            cache[packet_num] = data;

            // ACK 받을 때까지 재전송
            send_and_wait_ack(sock_fd,
                              data,
                              srv,
                              "ACK_LOG:" + std::to_string(packet_num),
                              cache,
                              packet_num);
            ++packet_num;
        }

        // --- 2-2. 서버로부터 제어 명령 수신 ---
        ssize_t n;
        while ((n = recvfrom(sock_fd, buf, sizeof(buf)-1,
                             0, nullptr, nullptr)) > 0)
        {
            buf[n] = '\0';
            auto cmd_pkt = json::parse(buf);
            if (cmd_pkt["type"] == "cmd") {
                std::string action = cmd_pkt["action"];
                if (action == "pause") {
                    cmd_q.Produce(0);   // 예: 0 = pause
                    log_msg("INFO", "Pause 명령 수신");
                }
                else if (action == "unlock") {
                    cmd_q.Produce(1);   // 예: 1 = unlock
                    log_msg("INFO", "Unlock 명령 수신");
                }
                else if (action == "return") {
                    cmd_q.Produce(2);   // 예: 2 = return
                    log_msg("INFO", "Return 명령 수신");
                }
            }
        }

        // 100ms 주기
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 스레드 종료 시 자원 정리
    close(sock_fd);
}
