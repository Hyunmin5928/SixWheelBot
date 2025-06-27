#include "motor_module.h"
/*
    command key :   straight stop rotate avoid backoff 
*/

void motor_thread(
    const std::string& port,
    unsigned int baud,
    SafeQueue<float>& dir_queue_g,
    SafeQueue<float>& dir_queue_v,
    SafeQueue<LaserPoint>& point_queue
){
    pthread_setname_np(pthread_self(),"[THREAD] command_D");
    if (!g_serial.Open(port.c_str(), baud)) {
        std::ostringstream oss;
        oss << "[IMU] Failed to open " << port << "@" << baud;
        Logger::instance().error("imu", oss.str());
        running.store(false);
    }
    Logger::instance().info("motor","[MOTOR] Command Thread start");
    std::string cmd="";
    float dir_g, dir_v;
    LaserPoint pnt;
    char linebuf[128];

    while(run_motor.load()){    
        if (point_queue.ConsumeSync(pnt)) {
            //장애물 위치(각도)와 거리 파악
            float dist  = pnt.range;
            float angle = pnt.angle;
            std::ostringstream oss;
            oss << "[MOTOR] Obstacle detected: dist="  << std::to_string(dist)
                << "cm, angle=" << std::to_string(angle);
            Logger::instance().info("motor", oss.str());
            // 거리가 0(감지불가 임계값 이하)일 경우 무시, 거리와 각도가 회피 기준값 이내로 들어오면 회피 동작
            // std::cout<<"값 받음 :"<<dist<<" "<<angle<<"\n";
            if (dist > 10.0f
             && dist <= OBSTACLE_DISTANCE_THRESHOLD
             && std::fabs(angle) <= OBSTACLE_ANGLE_LIMIT)
            {
                std::ostringstream oss;
                oss << "[MOTOR] Obstacle detected: dist="  << std::to_string(dist)
                << "cm, angle=" << std::to_string(angle);
                Logger::instance().warn("motor", oss.str());
                cmd="avoid ";
                cmd+=std::to_string(dist)+" "+std::to_string(angle)+"\n";
                g_serial.Write(cmd.c_str(), cmd.size());
                continue; // 장애물 처리 후 다음 루프
                // 만약 회피 기동 중에 또 다른 장애물이 발견될 경우..? 이에 대한 대처가 존재하지 않음.. 단일 장애물 기준
                // 장애물 회피할 때 인도 끝자락에 있을 경우 도로로 떨어질 가능성 있음 
                //     >> 인도 폭 인식하여 너무 끝단에 가지 않는 방향으로 조정하여 회피하도록 해야함
            }
        }
        // 2순위 네비게이션 방향 처리
        else if (dir_queue_g.ConsumeSync(dir_g) && 
                !point_queue.ConsumeSync(pnt)){
            if(dir_g==0.0f)
            {
                Logger::instance().info("motor", "[MOTOR] send straight");
                cmd="straight\n";
            }
            else
            {
                cmd="rotate ";
                std::string msg = "[MOTOR] g send rotate ";
                msg+=dir_g;
                Logger::instance().info("motor", msg);
                cmd+=std::to_string(dir_g)+"\n";
            }
            g_serial.Write(cmd.c_str(), cmd.size());
        }

        // 3순위 비전 방향 처리
        else if(dir_queue_v.ConsumeSync(dir_v) &&
                !point_queue.ConsumeSync(pnt)){
            std::string msg = "[MOTOR] v send rotate ";
            msg+=dir_v;
            Logger::instance().info("motor", msg);
            cmd="rotate ";
            cmd+=std::to_string(dir_v)+"\n";
            g_serial.Write(cmd.c_str(),cmd.size());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    dir_queue_g.Finish();
    dir_queue_v.Finish();
    point_queue.Finish();
}