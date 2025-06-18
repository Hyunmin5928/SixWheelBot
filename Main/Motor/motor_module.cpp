#include "motor_module.h"

// 외부에서 선언된 전역 플래그
extern std::atomic<bool> running;

// 모듈 진입점
void motor_thread(
    SafeQueue<std::vector<LaserPoint>>& lidar_q,
    SafeQueue<int>&                    cmd_q)
{
    Motor motor;

    while (running) {
        std::vector<LaserPoint> scans;
        if (lidar_q.ConsumeSync(scans)) {
            // 가장 가까운 스캔 포인트 찾기
            float minDist = std::numeric_limits<float>::infinity();
            float minAng  = 0;
            for (auto &p : scans) {
                if (p.range > 0 && p.range < minDist) {
                    minDist = p.range;
                    minAng  = p.angle;
                }
            }
            // 회피 로직
            if (minDist < avoidDistance_trigger) {
                motor.curve_avoid(minDist, 700, minAng);
            }
            else {
                motor.straight(500);
            }
        }
        // 외부 명령 (pause/unlock/return) 처리
        int cmd;
        if (cmd_q.Consume(cmd)) {
            switch (cmd) {
                case 0:  // pause
                    motor.stop();
                    std::cout << "[motor] PAUSE\n";
                    break;
                case 1:  // unlock (예시)
                    motor.curve_avoid( nearestDist, 300, 0.0f );
                    std::cout << "[motor] UNLOCK 행동\n";
                    break;
                case 2:  // return
                    motor.backoff(400);
                    std::cout << "[motor] RETURN 행동\n";
                    break;
                default:
                    break;
            }
        }

        // 10Hz 루프
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 종료 시 모터 정지
    motor.stop();
    // 큐에 더 이상 값이 들어오지 않음을 알림;
    cmd_q.Finish();
}
