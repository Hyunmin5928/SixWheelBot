#include "lidar_module.h"
// 전역 종료 플래그

// LiDAR 결과를 전달하는 큐
// SafeQueue<std::vector<ScanPoint>>& lidar_q
void lidar_thread(SafeQueue<LaserPoint>& lidar_q) {
    Lidar lidar;
    bool first = true;
    while (running.load()) {
        if (run_lidar.load()) {
            if (first){
                lidar.turnOn();
                first = false;
            }
            lidar.scan_oneCycle();  // 계속 데이터를 갱신해놓기
            LaserPoint scans = lidar.get_nearPoint();
            std::ostringstream msg;
            msg << std::fixed << std::setprecision(2);
            msg << "[LiDAR Near Point] ang : " << scans.angle
                << ", range : " << scans.range;
            Logger::instance().info("lidar", msg.str());
            lidar_q.Produce(std::move(scans));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    lidar.turnOff();  // 종료 시 끔
    lidar_q.Finish();
}

// void lidar_thread(
//     SafeQueue<LaserPoint>& lidar_q)
// {
//     Lidar lidar;
//     // 초기화 등 필요시 수행
//     while (running.load()) {
//         if(run_lidar.load()) {
//             lidar.scan_oneCycle();
//             // 여기서 nearPoint.clear() 때문에 쓰레기 값이 발생함. get_nearpoint가 계산될 때까지 딜레이가 되도록 해야할듯 if(lidar.get_nearPoint())
//             auto scans = lidar.get_nearPoint();
//             std::ostringstream msg;
//             msg << std::fixed << std::setprecision(2);  // 소수점 둘째 자리까지
//             msg << "[LiDAR Near Point] ang : " << scans.angle
//                 << ", range : " << scans.range;
//             Logger::instance().info("lidar", msg.str());
//             lidar_q.Produce(std::move(scans));
//         }
//         // start→stop 전환 시 여기서 아무 일도 하지 않고 깨어 있도록
//         std::this_thread::sleep_for(std::chrono::milliseconds(50));
//     }
//     // 종료 시 대기 중인 ConsumeSync 해제
//     lidar_q.Finish();
// }

// void lidar_test_producer(
//     SafeQueue<LaserPoint>& lidar_q
// ){
//     while(running){
//         LaserPoint p;
//         p.angle=0.0f;
//         p.range=230.0f;
//         p.intensity=3.0f;

//         lidar_q.Produce(std::move(p));
//         std::this_thread::sleep_for(std::chrono::microseconds(100));
//     }
//     lidar_q.Finish();
// }
