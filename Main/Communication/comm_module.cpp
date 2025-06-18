#include "comm_module.h"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <chrono>
#include <thread>
#include <nlohmann/json.hpp>
#include <map>

using json = nlohmann::json;

void send_and_wait_ack(int sock, const std::string& data,
                       const sockaddr_in& srv_addr,
                       const std::string& ack_str,
                       std::map<int,std::string>& cache,
                       int key);
extern void log_msg(const std::string&, const std::string&);

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
            sock_fd, data,
            srv,
            "ACK_LOG:" + std::to_string(packet_num),
            cache, packet_num);
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

void comm_thread(SafeQueue<std::pair<double,double>>& gps_q,
                 SafeQueue<int>&                      cmd_q,
                 SafeQueue<std::string>&              log_q) {
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in cli{}, srv{};
    cli.sin_family = AF_INET;
    cli.sin_port   = htons(CLIENT_PORT);
    cli.sin_addr.s_addr = inet_addr(CLIENT_IP.c_str());
    bind(sock_fd, (sockaddr*)&cli, sizeof(cli));
    srv.sin_family = AF_INET;
    srv.sin_port   = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP.c_str(), &srv.sin_addr);
    fcntl(sock_fd, F_SETFL, O_NONBLOCK);

    std::thread ts(log_sender_thread, std::ref(log_q));
    std::thread tr(cmd_receiver_thread, std::ref(cmd_q));

    while (running) {
        std::pair<double,double> pos;
        if (gps_q.Consume(pos)) {
            json pkt = {
                {"type","log"},
                {"gps",{pos.first,pos.second}},
                {"timestamp",int(std::time(nullptr))}
            };
            log_q.Produce(pkt.dump());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    ts.join(); tr.join();
    close(sock_fd);
    gps_q.Finish(); cmd_q.Finish(); log_q.Finish();
}