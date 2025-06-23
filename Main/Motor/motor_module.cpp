#include "motor_module.h"

std::atomic<bool> running{true};

void motor_test_thread(
    SafeQueue<LaserPoint>& point_queue,
    SafeQueue<float>& yaw_queue
){
    Motor motor;
    Logger::instance().info("motor", "[motor_module] Motor Thread start");

    while(running){

    }
}

void motor_thread(
    SafeQueue<float>&    dir_queue,
    SafeQueue<LaserPoint>& point_queue,
    SafeQueue<float>& yaw_queue,
    SafeQueue<bool>& arrive_queue
) {
    Motor motor;
    Logger::instance().info("motor", "[motor_module] Motor Thread start");
    bool is_arrive=false;
    // 이 스레드가 돌아가는 중이고, 아직 도착하지 않았다면
    while (running && !is_arrive) {
        LaserPoint pnt;
        //  도착 여부 확인
        arrive_queue.ConsumeSync(is_arrive);
        //  yaw값은 항상 motor.curDgr로 업데이트하도록 
        //  (non blocking 방식인 consume을 사용하여 rotate함수가 도는 와중에도 지속적으로 업데이트가 되도록 함)
        yaw_queue.Consume(motor.curDgr);
        //도착했으면 스레드 종료
        if(is_arrvie) break;

        // 1순위 : 장애물 회피
        if (point_queue.ConsumeSync(pnt)) {
            //장애물 위치(각도)와 거리 파악
            float dist  = pnt.range;
            float angle = pnt.angle;
            // 거리가 0(감지불가 임계값 이하)일 경우 무시, 거리와 각도가 회피 기준값 이내로 들어오면 회피 동작
            std::cout<<"값 받음 :"<<dist<<" "<<angle<<"\n";
            if (dist > 0.0f
             && dist <= OBSTACLE_DISTANCE_THRESHOLD
             && std::fabs(angle) <= OBSTACLE_ANGLE_LIMIT)
            {
                std::ostringstream oss;
                oss << "[motor_module] Obstacle detected: dist="  << std::to_string(dist)
                << "cm, angle=" << std::to_string(angle);
                Logger::instance().warn("motor", oss.str());
                motor.curve_avoid(dist, DEFAULT_PWM, angle, false);
                continue; // 장애물 처리 후 다음 루프
                // 만약 회피 기동 중에 또 다른 장애물이 발견될 경우..? 이에 대한 대처가 존재하지 않음.. 단일 장애물 기준
                // 장애물 회피할 때 인도 끝자락에 있을 경우 도로로 떨어질 가능성 있음 
                //     >> 인도 폭 인식하여 너무 끝단에 가지 않는 방향으로 조정하여 회피하도록 해야함
            }
        }
        // 2순위 내비게이션 방향 처리
        float dir;
        if (dir_queue.ConsumeSync(dir)) {

            if(dir==0.0f)
            {
                Logger::instance().info("motor", "[motor_module] straight");
                motor.straight(DEFALUT_PWM);
            }
            else
            {
                string msg = "[motor_module] rotate ";
                msg+=dir;
                Logger::instance().info("motor", msg);
                motor.rotate(DEFAULT_PWM, dir);
            }
            /*
            switch (dir) {
                case 0:  // pause
                    Logger::instance().info("motor", "[motor_module] stop");
                    motor.stop();
                    break;
                case 1:  // forward
                    
                    break;
                case 2:  // rotate right (예시)
                    
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
            }*/
        }
        // 10Hz 루프

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 종료 시 모터 정지
    motor.stop();
    // 큐에 더 이상 값이 들어오지 않음을 알림;
    dir_queue.Finish();
    point_queue.Finish();
    yaw_queue.Finish();
}