#include "lidar_module.h"
#include <pthread.h>
// 전역 종료 플래그

// 1) 스캔만 담당: raw_scan_queue 에 vector<LaserPoint> 생산
void lidar_producer(SafeQueue<LaserPoint>& lidar_queue) {
    pthread_setname_np(pthread_self(), "[THREAD]LIDAR_PROD");

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        return;
    }
    //192.168.61.139
    // 2. 수신할 주소 및 포트 설정
    sockaddr_in my_addr{};
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;     // 모든 인터페이스에서 수신
    my_addr.sin_port = htons(5005);           // LiDAR Pi가 보내는 포트

    // 3. 바인딩
    if (bind(sockfd, (sockaddr*)&my_addr, sizeof(my_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        return;
    }

    // 타임아웃 설정 (옵션: 블록 방지)
    timeval tv{1, 0}; // 1초 대기
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // constexpr int MAX_POINTS = 1000;
    //LaserPoint point;
    char buffer[256];
    sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);
    while (running.load()) {
        ssize_t len = recvfrom(sockfd, buffer, sizeof(buffer)-1, 0,
                        (sockaddr*)&sender_addr, &sender_len);
        // 1) 오류(-1) 처리
        if (len == -1) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // 타임아웃 또는 데이터 없음: 잠깐 대기 후 루프 유지
                continue;
            }
            perror("[THREAD]LIDAR_PROD recvfrom failed");
            continue;
        }
        if (len > 0) {
            buffer[len] = '\0'; // null-terminate
            std::string received_str(buffer);

            LaserPoint point;
            std::istringstream iss(received_str);
            iss >> point.angle >> point.range;
            point.intensity=0.0f;

            lidar_queue.Produce(std::move(point));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    close(sockfd);
    lidar_queue.Finish();
}