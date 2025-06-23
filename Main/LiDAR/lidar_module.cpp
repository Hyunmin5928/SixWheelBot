#include "lidar_module.h"
// 전역 종료 플래그

// LiDAR 결과를 전달하는 큐
// SafeQueue<std::vector<ScanPoint>>& lidar_q
void lidar_thread(
    SafeQueue<std::vector<LaserScan>>& lidar_q)
{
    Lidar lidar;
    // 초기화 등 필요시 수행

    while (running) {
        lidar.scan_oneCycle();
        auto scans = lidar.get_scanData();
        // 형식 일치 필요
        // lidar_q.Produce(std::move(scans));

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    // 종료 시 대기 중인 ConsumeSync 해제
    lidar_q.Finish();
}
