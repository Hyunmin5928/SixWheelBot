#include "comm_module.h"

void send_and_wait_ack(int sock, 
                       const std::string& data,
                       const sockaddr_in& srv_addr,
                       const std::string& ack_str,
                       std::map<int,std::string>& cache,
                       int key)
{
    int orig_flags = fcntl(sock, F_GETFL, 0);

    fcntl(sock, F_SETFL, orig_flags & ~O_NONBLOCK);

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
                fcntl(sock, F_SETFL, orig_flags);
                return;
            }
        }
        // log_msg("WARNING", ack_str + " retry " + std::to_string(i+1));
        Logger::instance().warn("comm", ack_str + " retry " + std::to_string(i+1));
    }
    // 3) 원래대로 non-blocking 모드 복원
    fcntl(sock, F_SETFL, orig_flags);
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
            buf[n] = '\0';
            std::string msg(buf);
            // 1) 비-JSON 메시지(ACK, RETRANS 등)은 무시
            if (msg.empty() || msg.front() != '{') {
                continue;
            }
            // 2) JSON 파싱 & cmd 타입만 처리
            try {
                auto pkt = json::parse(msg);
                if (pkt.contains("type") && pkt["type"] == "cmd") {
                    std::string act = pkt["action"];
                    int code = (act=="pause"?0
                              : act=="unlock"?1
                              : act=="return"?2
                              : -1);
                    if (code >= 0) {
                        cmd_q.Produce(std::move(code));
                    }
                }
            } catch (const json::parse_error& e) {
                // 혹시라도 형식이 이상하면 무시
                Logger::instance().warn("comm",
                    std::string("cmd_receiver_thread parse error: ") + e.what());
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
    // 1) 소켓 생성·바인드
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in cli{}, srv{};
    cli.sin_family      = AF_INET;
    cli.sin_port        = htons(CLIENT_PORT);
    cli.sin_addr.s_addr = (CLIENT_IP=="0.0.0.0"
                           ? INADDR_ANY
                           : inet_addr(ALLOW_IP.c_str()));
    bind(sock_fd, (sockaddr*)&cli, sizeof(cli));
    srv.sin_family = AF_INET;
    srv.sin_port   = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP.c_str(), &srv.sin_addr);

    // 논블로킹 모드
    fcntl(sock_fd, F_SETFL, O_NONBLOCK);

    // 2) 최초 map 수신 (블록킹처럼 동작)
    char buf[65536];
    std::string msg;
    // std::vector<std::vector<double>> raw_route;
    while (running) {
        ssize_t len = recvfrom(
            sock_fd,
            buf, sizeof(buf)-1,
            0, nullptr, nullptr
        );  // 서버→수신
        if (len <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        buf[len] = '\0';
        msg = buf;
        // JSON 아닌 데이터면 스킵
        if (msg.empty() || msg.front() != '{') {
            Logger::instance().warn("comm", "[comm_thread] Non-JSON during map recv: " + msg);
            continue;
        }
        // 파싱 시도
        nlohmann::json pkt;
        try {
            pkt = nlohmann::json::parse(msg);
        } catch (const nlohmann::json::parse_error& e) {
            Logger::instance().warn("comm", std::string("[comm_thread] JSON parse error: ") + e.what());
            continue;
        }
        Logger::instance().info("comm", "[comm_thread] Comm Thread Map recv");
        if (pkt["type"] == "map") {
            // 객체 배열 파싱
            std::vector<std::tuple<double,double,int>> route;
            for (auto& elem : pkt["route"]) {
                double lat = elem.at("lat").get<double>();
                double lon = elem.at("lon").get<double>();
                int    dir = elem.at("dir").get<int>();
                route.emplace_back(lat, lon, dir);
            }
            map_q.Produce(std::move(route));
            // ACK_MAP 전송
            const char* ack = "ACK_MAP:0";
            sendto(
                  sock_fd,
                  ack, strlen(ack),
                  0,
                  (sockaddr*)&srv, 
                  sizeof(srv)
            );
            // log_msg("INFO", "Map data received and ACK sent");
            Logger::instance().info("comm", "[comm_thread]Map data received and ACK sent");
            run_imu.store(true);
            run_gps.store(true);
            run_lidar.store(true);
            break;
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

void gps_sender_thread(SafeQueue<std::pair<double,double>>& gps_q) {
    // 서버 주소 셋업
    sockaddr_in srv{};
    srv.sin_family = AF_INET;
    srv.sin_port   = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP.c_str(), &srv.sin_addr);

    std::map<int, std::string> cache;
    int packet_num = 0;
    Logger::instance().info("comm", "[gps_sender_thread] GPS sender Thread start");
    std::pair<double,double> pos;
    while (running) {
        if (gps_q.ConsumeSync(pos)){
            // 1) JSON 패킷 생성
            json pkt;
            pkt["packet_number"] = packet_num;
            pkt["type"]          = "log";  // 서버가 log 타입으로 처리하게
            // UNIX epoch 기준 초(소수점 포함)
            pkt["timestamp"]     = std::chrono::duration<double>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
            // GPS 정보
            pkt["gps"] = {
                {"lat", pos.first},
                {"lon", pos.second}
            };
            // (선택) 로봇 상태가 필요하면 여기에 추가
            // pkt["robot_state"] = "MOVING";

            std::string data = pkt.dump();
            Logger::instance().info("comm", "[gps_sender_thread] Packet Data : " + data);
            // 2) 캐싱 후 전송 & ACK 대기
            cache[packet_num] = data;
            send_and_wait_ack(
                sock_fd,
                data,
                srv,
                "ACK_LOG:" + std::to_string(packet_num),
                cache,
                packet_num
            );
            ++packet_num;
            Logger::instance().info("comm", "[gps_sender_thread] GPS sender Thread sending");
            // 전송 주기 조절 (예: 1Hz)
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

}