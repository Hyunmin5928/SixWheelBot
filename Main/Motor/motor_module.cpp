#include "motor_module.h"

void motor_thread(
    SafeQueue<GpsDir>&    dir_queue,
    SafeQueue<LaserPoint>& point_queue,
    SafeQueue<float>& m_yaw_q
) {
    Motor motor;
    Logger::instance().info("motor", "[motor_module] Motor Thread start");

    while (running) {
        LaserPoint pnt;
        m_yaw_q.Consume(motor.curDgr);
        if (point_queue.ConsumeSync(pnt)) {
            float dist  = pnt.range;
            float angle = pnt.angle;
            if (dist > 0 
             && dist <= OBSTACLE_DISTANCE_THRESHOLD
             && std::fabs(angle) <= OBSTACLE_ANGLE_LIMIT)
            {
                std::ostringstream oss;
                oss << "[motor_module] Obstacle detected: dist="  << std::to_string(dist)
                << "cm, angle=" << std::to_string(angle);
                Logger::instance().warn("motor", oss.str());
                motor.curve_avoid(dist, DEFAULT_PWM, angle, false);
                continue; // 장애물 처리 후 다음 루프
            }
        }
        // 2) 내비게이션 방향 처리
        GpsDir dir;
        if (dir_queue.ConsumeSync(dir)) {
            switch (dir) {
                case 0:  // pause
                    Logger::instance().info("motor", "[motor_module] stop");
                    motor.stop();
                    break;
                case 1:  // forward
                    Logger::instance().info("motor", "[motor_module] straight");
                    motor.straight(DEFAULT_PWM);
                    break;
                case 2:  // rotate right (예시)
                    Logger::instance().info("motor", "[motor_module] rotate +90deg");
                    motor.rotate(DEFAULT_PWM,  90.0f);
                    break;
                case 3:  // rotate left (예시)
                    Logger::instance().info("motor", "[motor_module] rotate -90deg");
                    motor.rotate(DEFAULT_PWM, -90.0f);
                    break;
                default:
                    std::ostringstream oss;
                    oss << "[motor_module] Unknown dir code: "  << std::to_string(dir);
                    Logger::instance().info("motor", oss.str());
                    motor.stop();
            }
        }
        // 10Hz 루프
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 종료 시 모터 정지
    motor.stop();
    // 큐에 더 이상 값이 들어오지 않음을 알림;
    dir_queue.Finish();
    point_queue.Finish();
    m_yaw_q.Finish();
}
