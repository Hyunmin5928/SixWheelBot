#include "lidar_module.h"
// 전역 종료 플래그

// LiDAR 결과를 전달하는 큐
// SafeQueue<std::vector<ScanPoint>>& lidar_q
void lidar_thread(
    SafeQueue<LaserPoint>& lidar_q)
{
    Lidar lidar;
    // 초기화 등 필요시 수행
    while (running.load()) {
        if(run_lidar.load()) {
            lidar.scan_oneCycle();
            auto scans = lidar.get_nearPoint();
            Logger::instance().info("lidar", "[LiDAR] Scannaing oneCycle");
            lidar_q.Produce(std::move(scans));
        }
        // start→stop 전환 시 여기서 아무 일도 하지 않고 깨어 있도록
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    // 종료 시 대기 중인 ConsumeSync 해제
    lidar_q.Finish();
}

void lidar_test_producer(
    SafeQueue<LaserPoint>& lidar_q
){
    while(running){
        LaserPoint p;
        p.angle=0.0f;
        p.range=230.0f;
        p.intensity=3.0f;

        lidar_q.Produce(std::move(p));
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    lidar_q.Finish();
}
