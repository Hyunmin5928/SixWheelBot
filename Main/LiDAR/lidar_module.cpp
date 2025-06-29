#include "lidar_module.h"
#include <pthread.h>
// 전역 종료 플래그

// 1) 스캔만 담당: raw_scan_queue 에 vector<LaserPoint> 생산
void lidar_producer() {
    pthread_setname_np(pthread_self(), "[THREAD]LIDAR_PROD");
    Lidar lidar;
    lidar.turnOn();

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
    }
    //192.168.61.139
    // 2. 수신할 주소 및 포트 설정
    sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;     // 모든 인터페이스에서 수신
    my_addr.sin_port = htons(5005);           // LiDAR Pi가 보내는 포트

    // 3. 바인딩
    if (bind(sockfd, (sockaddr*)&my_addr, sizeof(my_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        return;
    }

    constexpr int MAX_POINTS = 1000;
    std::vector<LaserPoint> point(MAX_POINTS);
    sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);
    
    while (running.load()) {

        ssize_t len = recvfrom(sockfd, point.data(), point.size() * sizeof(LaserPoint), 0,
                               (sockaddr*)&sender_addr, &sender_len);

        if (len % sizeof(LaserPoint) == 0 && len > 0) {
            int count = len / sizeof(LaserPoint);
            point.resize(count);
            std::cout << "Received " << count << " LaserPoints" << std::endl;
            raw_scan_queue.Produce(std::move(point));
        } else {
            std::cerr << "Received unexpected packet size: " << len << " bytes" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    lidar.turnOff();
    raw_scan_queue.Finish();
    close(sockfd);
}

// 2) 계산만 담당: raw_scan_queue 에서 뽑아 nearest point 계산 → lidar_queue 로 소비
void lidar_consumer(SafeQueue<std::vector<LaserPoint>>& in_q,
                    SafeQueue<LaserPoint>& out_q) {
    std::vector<LaserPoint> pts;
    pthread_setname_np(pthread_self(), "[THREAD]LIDAR_COND");
    while (in_q.ConsumeSync(pts)) {                       // 블록 대기
        // 가장 가까운 점 계산 (기존 get_nearPoint 로직 분리)
        LaserPoint nearPoint;
        float minR = std::numeric_limits<float>::infinity();
        for (auto &p : pts) {
            if (p.range > 0 && p.range < minR &&
               p.angle > -60.0f && p.angle < 60.0f) {
                minR = p.range;
                nearPoint = p;
            }
        }
        std::ostringstream msg;
        msg << std::fixed << std::setprecision(2);
        msg << "[LiDAR Near Point] ang : " << nearPoint.angle
            << ", range : " << nearPoint.range;
        Logger::instance().info("lidar", msg.str());

        out_q.Produce(std::move(nearPoint));                  // 최종 결과 생산
    }
    out_q.Finish();
}