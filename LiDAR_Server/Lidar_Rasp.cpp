#include "Lidar.h"

int main(){
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in main_addr;
    memset(&main_addr, 0, sizeof(main_addr));
    main_addr.sin_port = htons(5005); // main pi 에서 받을 포트
    inet_pton(AF_INET, "10.67.230.232", &main_addr.sin_addr); //데이터 못받았던 게 이쪽 문제였던 것으로 예상
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

        //sendto 문제 가능성 있음 (쓰레기값이라 조건문 통과 못할 가능성 있음)
        //sendto(sockfd, &nearPoint, sizeof(LaserPoint),0, (sockaddr*)&main_addr, sizeof(main_addr));
        //기존 struct형으로 보내던 것 >> string으로 변형하여 보내보도록 함
        std::string sendstr = std::to_string(nearPoint.angle)+" "+std::to_string(nearPoint.range)+"\n";
        sendto(sockfd, sendstr.c_str(), sizeof(sendstr),0, (sockaddr*)&main_addr, sizeof(main_addr));
        usleep(10000);
    }
    close(sockfd);
    lidar.turnOff();
    return 0;
}
