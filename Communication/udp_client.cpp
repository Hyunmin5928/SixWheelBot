#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <thread>
#include <csignal>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// 전역 설정
std::string SERVER_IP;
int SERVER_PORT;
std::string CLIENT_IP;
int CLIENT_PORT;
std::string ALLOW_IP;
double ACK_TIMEOUT;
int RETRY_LIMIT;
std::string LOG_FILE;

const char* PID_FILE = "/var/run/udp_client.pid";
int log_fd;

void log_msg(const std::string& level, const std::string& msg) {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S")
        << " [" << level << "] " << msg << "\n";
    write(log_fd, oss.str().c_str(), oss.str().size());
}

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

void daemonize() {
    pid_t pid = fork();
    if (pid > 0) exit(0);
    setsid();
    pid = fork();
    if (pid > 0) exit(0);
    umask(0);
    // pid 파일 작성
    std::ofstream pidf(PID_FILE);
    pidf << getpid();
    pidf.close();
    signal(SIGTERM, [](int){ unlink(PID_FILE); exit(0); });
}

void send_and_wait_ack(int sock, const std::string& data, const sockaddr_in& srv_addr,
                       const std::string& ack_str, std::map<int,std::string>& cache, int key) {
    for (int i = 0; i < RETRY_LIMIT; ++i) {
        sendto(sock, data.data(), data.size(), 0, (sockaddr*)&srv_addr, sizeof(srv_addr));
        char buf[1024];
        struct timeval tv{(int)ACK_TIMEOUT, int((ACK_TIMEOUT - int(ACK_TIMEOUT))*1e6)};
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

void client_loop() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in cli_addr{}, srv_addr{};
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(CLIENT_PORT);
    cli_addr.sin_addr.s_addr = (ALLOW_IP == "0.0.0.0" ? INADDR_ANY : inet_addr(ALLOW_IP.c_str()));
    bind(sock, (sockaddr*)&cli_addr, sizeof(cli_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP.c_str(), &srv_addr.sin_addr);

    // 로그 메시지 직전에 IP 문자열로 변환
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &cli_addr.sin_addr, ip_str, sizeof(ip_str));
    // 리터럴을 std::string으로 감싸기
    std::string msg = std::string("Client listening on ") + ip_str + ":" + std::to_string(CLIENT_PORT);
    log_msg("INFO", msg);

    std::map<int,std::string> packet_cache;
    bool received_map = false;
    char buf[65536];

    while (true) {
        ssize_t len = recvfrom(sock, buf, sizeof(buf)-1, 0, nullptr, nullptr);
        if (len < 0) continue;
        buf[len] = '\0';
        std::string msg(buf);
        // Retrans requests
        if (msg.rfind("RETRANS_MAP:",0)==0) {
            sendto(sock, packet_cache[0].data(), packet_cache[0].size(), 0,
                   (sockaddr*)&cli_addr, sizeof(cli_addr));
            log_msg("INFO","Retransmitted map"); continue;
        }
        if (msg.rfind("RETRANS_LOG:",0)==0) {
            int num = std::stoi(msg.substr(12));
            sendto(sock, packet_cache[num].data(), packet_cache[num].size(), 0,
                   (sockaddr*)&srv_addr, sizeof(srv_addr));
            log_msg("INFO","Retransmitted log " + std::to_string(num)); continue;
        }
        if (msg.rfind("RETRANS_DONE:",0)==0) {
            int num = std::stoi(msg.substr(12));
            sendto(sock, packet_cache[num].data(), packet_cache[num].size(), 0,
                   (sockaddr*)&srv_addr, sizeof(srv_addr));
            log_msg("INFO","Retransmitted done " + std::to_string(num)); continue;
        }
        if (msg.front()!='{') continue;
        auto pkt = json::parse(msg);
        std::string ptype = pkt["type"];
        int num = pkt["packet_number"];
        if (ptype=="map" && !received_map) {
            packet_cache[0] = msg;
            sendto(sock,"ACK_MAP:0",9,0,(sockaddr*)&cli_addr,sizeof(cli_addr));
            auto route = pkt["route"];
            received_map = true;
            log_msg("INFO","Map received: " + std::to_string(route.size()) + " points");
            // send logs
            for (size_t i=0;i<route.size();++i) {
                json log_pkt = { {"packet_number", (int)i}, {"type","log"},
                    {"robot_state","moving"}, {"gps",route[i]}, {"timestamp",time(nullptr)} };
                std::string data_log = log_pkt.dump();
                packet_cache[i] = data_log;
                send_and_wait_ack(sock, data_log, srv_addr, "ACK_LOG:"+std::to_string(i), packet_cache, i);
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
            // done
            int done_num = route.size();
            json done_pkt = { {"packet_number", done_num}, {"type","done"}, {"timestamp",time(nullptr)} };
            std::string data_done = done_pkt.dump();
            packet_cache[done_num] = data_done;
            send_and_wait_ack(sock, data_done, srv_addr, "ACK_DONE:"+std::to_string(done_num), packet_cache, done_num);
            log_msg("INFO","Delivery completed");
            break;
        }
    }
    close(sock);
}

int main(int argc, char* argv[]) {
    load_config("config/config.json");
    // 로그 파일 열기
    log_fd = open(LOG_FILE.c_str(), O_CREAT|O_APPEND|O_WRONLY, 0644);

    if (argc!=2) { std::cout<<"Usage: "<<argv[0]<<" [start|stop|restart|status]\n"; return 1;}  
    std::string cmd = argv[1];
    if (cmd=="start") {
        log_msg("INFO","Client Daemon start");
        daemonize();
        client_loop();
    } 
    else if (cmd=="stop") {
        log_msg("INFO","Client Daemon stop");
        std::ifstream pidf(PID_FILE);
        pid_t pid; if (pidf>>pid) kill(pid,SIGTERM);
    } 
    else if (cmd=="restart") {
        log_msg("INFO","Client Daemon start");
        std::ifstream pidf(PID_FILE);
        pid_t pid; if (pidf>>pid) kill(pid,SIGTERM);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        daemonize(); client_loop();
    } 
    else if (cmd=="status") {
        if (access(PID_FILE,F_OK)==0)  {
            std::cout<<"UDP Client Daemon ";
            std::cout<<"Running\n";
        }
        else std::cout<<"Stopped\n";
    } 
    else {
        std::cout<<"Unknown command\n";
        std::cout<<"Usage: "<<argv[0]<<" [start|stop|restart|status]\n";
    }
    close(log_fd);
    return 0;
}
