#include "comm_module.h"
/*
    로그 함수 사용 법
    // 로그 초기화: 파일 경로, 최소 출력 레벨 지정
    util::Logger::instance().init("app.log", util::LogLevel::Debug);

    util::Logger::instance().info("프로그램 시작");
    util::Logger::instance().debug("디버그 메시지");
    util::Logger::instance().warn("경고 메시지");
    util::Logger::instance().error("오류 메시지");
*/
void send_and_wait_ack(int sock, 
                       const std::string& data,
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
        // log_msg("WARNING", ack_str + " retry " + std::to_string(i+1));
        Logger::instance().warn(ack_str + " retry " + std::to_string(i+1));
    }
}

void log_sender_thread(SafeQueue<std::string>& log_q) {
    sockaddr_in srv{};
    srv.sin_family = AF_INET;
    srv.sin_port   = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP.c_str(), &srv.sin_addr);

    std::map<int,std::string> cache;
    int packet_num = 0;
    std::string data;
    while (running && log_q.ConsumeSync(data)) {
        cache[packet_num] = data;
        send_and_wait_ack(
            sock_fd, 
            data,
            srv,
            "ACK_LOG:" + std::to_string(packet_num),
            cache, 
            packet_num);
        ++packet_num;
    }
}

void cmd_receiver_thread(SafeQueue<int>& cmd_q) {
    char buf[4096];
    sockaddr_in from{};
    socklen_t   fromlen = sizeof(from);
    while (running) {
        ssize_t n = recvfrom(sock_fd, buf, sizeof(buf)-1,
                             0, (sockaddr*)&from, &fromlen);
        if (n > 0) {
            buf[n]=0;
            auto pkt = json::parse(buf);
            if (pkt["type"] == "cmd") {
                std::string act = pkt["action"];
                int code = (act=="pause"?0: act=="unlock"?1: act=="return"?2:-1);
                if (code >= 0) cmd_q.Produce(std::move(code));
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void comm_thread(
    SafeQueue<std::vector<std::tuple<double,double,int>>>& map_q,
    SafeQueue<int>&                                        cmd_q,
    SafeQueue<std::string>&                                log_q)
{
    Logger::instance().init("app.log", LogLevel::Debug);
    // 1) 소켓 생성·바인드
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

    // 논블로킹 모드
    fcntl(sock_fd, F_SETFL, O_NONBLOCK);

    // 2) 최초 map 수신 (블록킹처럼 동작)
    char buf[65536];
    std::vector<std::vector<double>> raw_route;
    while (running) {
        ssize_t len = recvfrom(
            sock_fd,
            buf, sizeof(buf)-1,
            0, nullptr, nullptr
        );  // 서버→수신
        if (len > 0) {
            buf[len] = '\0';
            auto pkt = json::parse(buf);
            if (pkt["type"] == "map") {
                raw_route = pkt["route"]
                              .get<decltype(raw_route)>();
                // [lat, lon, dirCode] → 튜플 벡터로 변환
                std::vector<std::tuple<double,double,int>> route;
                for (auto &pt : raw_route) {
                    if (pt.size() >= 3) {
                        route.emplace_back(
                          pt[0], pt[1], int(pt[2])
                        );
                    }
                }
                // 공유 큐에 전달
                map_q.Produce(std::move(route));

                // ACK_MAP 전송
                const char* ack = "ACK_MAP:0";
                sendto(
                  sock_fd,
                  ack, strlen(ack),
                  0,
                  (sockaddr*)&srv, sizeof(srv)
                );
                // log_msg("INFO", "Map data received and ACK sent");
                Logger::instance().info("Map data received and ACK sent");
                break;
            }
        }
        // CPU 살짝 쉬어주기
        std::this_thread::sleep_for(
          std::chrono::milliseconds(50));
    }

    // 3) 로깅·명령 수신 전용 스레드 기동
    std::thread ts(log_sender_thread, std::ref(log_q));
    std::thread tr(cmd_receiver_thread, std::ref(cmd_q));

    // 4) 멀티스레드 종료 대기
    while (running) {
        std::this_thread::sleep_for(
          std::chrono::milliseconds(100));
    }

    ts.join();
    tr.join();
    close(sock_fd);

    // 5) 큐 종료 알림
    map_q.   Finish();
    cmd_q.   Finish();
    log_q.   Finish();
}