#include "command_module.h"

/*
    command key :   straight stop rotate avoid backoff 

    command에서는 cmd_queue를 내보냄, Motor 쪽에서 cmd_queue 값에 따라서 운전되도록 하면 됨
*/

void command_thread(
    SafeQueue<float> dir_queue,
    SafeQueue<LaserPoint>& point_queue,
    SafeQueue<ImuData>& imu_queue,
    SafeQueue<std::string>& cmd_queue
){
    pthread_setname_np(pthread_self(),"[THREAD] command_D");
    Logger::instance().info("[command]","[command_module] Command Thread start")
    std::string cmd="";
    float dir;
    LaserPoint pnt;
    ImuData imu;
    
    while(run_command.load()){    
        if(!imu_queue.ConsumeSync(imu)){
            Logger::instance().info("command","[command_module] Imu data didn't arrive");
        }else{
            //imu.yaw가 motor.curDgr로 업데이트 되어야함
        }

        if (point_queue.ConsumeSync(pnt)) {
            //장애물 위치(각도)와 거리 파악
            float dist  = pnt.range;
            float angle = pnt.angle;
            std::ostringstream oss;
            oss << "[command_module] Obstacle detected: dist="  << std::to_string(dist)
                << "cm, angle=" << std::to_string(angle);
            Logger::instance().info("command", oss.str());
            // 거리가 0(감지불가 임계값 이하)일 경우 무시, 거리와 각도가 회피 기준값 이내로 들어오면 회피 동작
            // std::cout<<"값 받음 :"<<dist<<" "<<angle<<"\n";
            if (dist > 10.0f
             && dist <= OBSTACLE_DISTANCE_THRESHOLD
             && std::fabs(angle) <= OBSTACLE_ANGLE_LIMIT)
            {
                std::ostringstream oss;
                oss << "[command_module] Obstacle detected: dist="  << std::to_string(dist)
                << "cm, angle=" << std::to_string(angle);
                Logger::instance().warn("command", oss.str());
                cmd="avoid";
                cmd_queue.Produce(std::move(cmd));
                
                continue; // 장애물 처리 후 다음 루프
                // 만약 회피 기동 중에 또 다른 장애물이 발견될 경우..? 이에 대한 대처가 존재하지 않음.. 단일 장애물 기준
                // 장애물 회피할 때 인도 끝자락에 있을 경우 도로로 떨어질 가능성 있음 
                //     >> 인도 폭 인식하여 너무 끝단에 가지 않는 방향으로 조정하여 회피하도록 해야함
            }
        }
    
        // 2순위 네비게이션 방향 처리
        if (dir_queue.ConsumeSync(dir)){
            if(dir==0.0f)
            {
                Logger::instance().info("command", "[command_module] send straight");
                cmd="straight";
                cmd_queue.Produce(std::move(cmd));
            }
            else
            {
                std::string msg = "[command_module] send rotate ";
                msg+=dir;
                Logger::instance().info("command", msg);
                cmd=std::to_string(dir);
                cmd_queue.Produce(std::move(cmd));
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    dir_queue.Finish();
    point_queue.Finish();
    imu_queue.Finish();
    cmd_queue.Finish();
}