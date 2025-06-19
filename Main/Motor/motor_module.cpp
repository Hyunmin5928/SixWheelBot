#include "motor_module.h"

// 모듈 진입점
void motor_thread(SafeQueue<std::tuple<int, float, float>>& m_cmd_q)
{
    Motor motor;
    std::tuple<int, float, float> cmdPair;
    while (running) {
        int cmd = 0;
        double dist = 0.0;
        if (m_cmd_q.Consume(cmdPair)) {

            switch (cmd) {
                case 0:  // pause
                    motor.stop();
                    std::cout << "[motor] PAUSE\n";
                    break;
                case 1:  // unlock (예시)
                    motor.curve_avoid(dist, 300, 0.0f );
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
    m_cmd_q.Finish();
}
