#include "motor_module.h"
#include <pthread.h>


void motor_thread(
    SafeQueue<std::string>& cmd_queue
    SafeQueue<LaserPoint> avoid_queue
) {
    pthread_setname_np(pthread_self(), "[THREAD]MOTOR_D");
    Motor motor;
    Logger::instance().info("motor", "[motor_module] Motor Thread start");
    int status=0;
    // 0 : 출발 1 : 도착 2: 복귀
    
    std::string cmd="";
    // 이 스레드가 돌아가는 중이고, 아직 도착하지 않았다면
    while (running.load()) {

        if(cmd_queue.ConsumeSync(cmd)){
            if (cmd  ==  "straight"){
                motor.straight(DEFAULT_PWM);
                Logger::instance().info("motor", "[motor_module] receive straight");
            }
            else if (cmd ==  "backoff"){
                motor.straight(DEFAULT_PWM);
                Logger::instance().info("motor", "[motor_module] receive backoff");
            }
            else if (cmd ==  "stop"){
                motor.stop();
                Logger::instance().info("motor", "[motor_module] receive stop");
            }
            else if (cmd   ==  "avoid"){
                LaserPoint pnt;
                if(avoid_queue.ConsumeSync(pnt)){
                    motor.curve_avoid(pnt.range, DEFAULT_PWM, pnt.angle);
                    Logger::instance().info("motor", "[motor_module] receive avoid");
                }
            }
            else{  // rotate
                // IMU 값에서 YAW를 추출 + 참고해서 회전해야함
                // IMU 값은 rotate 함수로 들어가서 curDgr로 업데이트되도록 해야함
                //g_serial.Read() 
                //  or 
                //  motor.curDgr = imu.yaw
                motor.rotate(DEFAULT_PWM, std::stof(cmd));
                Logger::instance().info("motor", "[motor_module] receive rotate");
            }
        }
    }

        
}