#include "Lidar.h"

int main(){
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in main_addr;
    memset(&main_addr, 0, sizeof(main_addr));
    main_addr.sin_port = htons(5005); // main pi 에서 받을 포트
    inet_pton(AF_INET, "192.168.61.232", &main_addr.sin_addr);
    // 라이다 라즈베리파이가 메인 라즈베리파이로 
    // 데이터를 계속 보내도록 해야함

    // 메인 라즈베리파이에서는 현재 running 중인지, 아닌지 확인
    // UDP
    Lidar lidar;
    lidar.turnOn();
    LaserPoint nearPoint;
    while(true) // running.load()
    {
        lidar.scan_oneCycle();
        auto pts = lidar.get_scanData();

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
        std::cout << msg.str() << std::endl;

        sendto(sockfd, &nearPoint, sizeof(LaserPoint),0, (sockaddr*)&main_addr, sizeof(main_addr));
        usleep(10000);
    }
    close(sockfd);
    lidar.turnOff();
    return 0;
}
