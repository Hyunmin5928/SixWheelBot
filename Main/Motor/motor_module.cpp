#include "motor_module.h"
/*
    command key :   straight stop rotate avoid backoff 
*/

void motor_thread(
    const std::string& port,
    unsigned int baud,
    SafeQueue<float>& dir_queue_g,
    SafeQueue<float>& dir_queue_v,
    SafeQueue<LaserPoint>& point_queue,
    SafeQueue<bool>&       m_stop_queue
){
    pthread_setname_np(pthread_self(),"[THREAD] command_D");
    if (!g_serial.Open(port.c_str(), baud)) {
        std::ostringstream oss;
        oss << "[motor] Failed to open " << port << "@" << baud;
        Logger::instance().error("motor", oss.str());
        running.store(false);
    }
    std::ostringstream oss;
    oss << "[MOTOR] Success to open " << port << "@" << baud;
    Logger::instance().info("motor", oss.str());
    Logger::instance().info("motor","[MOTOR] Command Thread start");
    std::string cmd="";
    std::string last_cmd="";
    float dir_g, dir_v;
    LaserPoint pnt;
    char linebuf[128];
    bool cmd_active = false;
    bool l_cmd      = false;
    bool g_cmd      = false;
    bool v_cmd      = false;
    bool got_l_queue = false;
    bool got_g_queue = false;
    bool got_v_queue = false;
    bool stop = false;

    while (true) {
        if (g_serial.ReadLine(linebuf, sizeof(linebuf))) {
            // 1) std::string 으로 복사
            std::string msg(linebuf);
            // 2) 뒤쪽에 붙은 \r, \n 전부 제거
            while (!msg.empty() && (msg.back() == '\r' || msg.back() == '\n')) {
                msg.pop_back();
            }

            // 로그 찍어 보기
            std::ostringstream oss;
            oss << "[MOTOR] Arduino send message trimmed: '" << msg << "'";
            Logger::instance().info("motor", oss.str());

            // 3) 순수 비교
            if (msg == "setup") {
                Logger::instance().info("motor", "[MOTOR] Arduino Setup done");
                run_motor.store(true);
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    while(running.load()){    
        // Logger::instance().info("motor", "[MOTOR] RUN loop");

        if(m_stop_queue.ConsumeSync(stop) && !cmd_active){
            if (stop){
                Logger::instance().info("motor", "[MOTOR] send stop");
                cmd="stop\n";
                g_serial.Write(cmd.c_str(), cmd.size());    
                run_motor.store(false);
            }
            else {
                // Logger::instance().info("motor", "[MOTOR] Return cmd recv");
                run_motor.store(true);
            }
        }

        if (g_serial.ReadLine(linebuf, sizeof(linebuf))) {
            // 1) std::string 으로 복사
            std::string msg(linebuf);
            while (!msg.empty() && (msg.back() == '\r' || msg.back() == '\n')) {
                msg.pop_back();
            }
            std::ostringstream oss;
            oss << "[MOTOR] Arduino send message trimmed: '" << msg << "'";
            Logger::instance().info("motor", oss.str());
            // "cmd_done" 을 받으면 cmd_active = false
            if (msg == "cmd_done") {
                cmd_active = false;
            }
        }
        if(run_motor.load() && !cmd_active){
            Logger::instance().info("motor", "[MOTOR] RUN_MOTOR loop");
            // 라이다 큐 들어왔을 경우
            got_l_queue = point_queue.ConsumeSync(pnt);
            if(got_l_queue){
                Logger::instance().info("motor", "[MOTOR] DIR queue received");
                //장애물 위치(각도)와 거리 파악
                float dist  = pnt.range;
                float angle = pnt.angle;
                // 거리가 0(감지불가 임계값 이하)일 경우 무시, 거리와 각도가 회피 기준값 이내로 들어오면 회피 동작
                // std::cout<<"값 받음 :"<<dist<<" "<<angle<<"\n";
                // 실제로 어떤 값이 할당되었는지 확인해보기 위해 로그 찍어놓음
                std::ostringstream oss;
                    oss << "[MOTOR] point_queue : dist="  << std::to_string(dist)
                    << "cm, angle=" << std::to_string(angle);
                    Logger::instance().warn("motor", oss.str());
                if (dist > 0.0f && dist <= OBSTACLE_DISTANCE_THRESHOLD)
                {
                    cmd_active = true;
                    std::ostringstream oss;
                    oss << "[MOTOR] Obstacle detected: dist="  << std::to_string(dist)
                    << "cm, angle=" << std::to_string(angle);
                    Logger::instance().warn("motor", oss.str());
                    cmd="avoid ";
                    cmd+=std::to_string(dist)+" "+std::to_string(angle)+"\n";
                    last_cmd = cmd;
                    g_serial.Write(cmd.c_str(), cmd.size());
                    //장애물 코드가 너무 자주 도는 것 방지
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
            }
            // GPS 큐 들어왔을 경우
            got_g_queue = dir_queue_g.ConsumeSync(dir_g);
            // GPS 큐는 항상 존재하므로, GPS 이외의 상황에서의 기동 상태가 아닐 경우 동작하도록 해야함
            if(got_g_queue && !cmd_active){
                if(dir_g==0.0f){
                    cmd="straight\n";
                    if(last_cmd != cmd){
                        Logger::instance().info("motor", "[MOTOR] send straight");
                        last_cmd = cmd;
                        g_serial.Write(cmd.c_str(), cmd.size());    
                    }
                }
                else{
                    // 이 경우는 rotate가 되어야하므로 cmd_active = true로 회전 완료까지는 다른 명령어 차단
                    cmd_active=true;
                    cmd="rotate ";
                    std::string msg = "[MOTOR] gps send rotate ";
                    msg+=std::to_string(dir_g);
                    Logger::instance().info("motor", msg);
                    cmd+=std::to_string(dir_g)+"\n";
                    last_cmd = cmd;
                    g_serial.Write(cmd.c_str(), cmd.size());
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            // Vision 큐 들어왔을 경우
            got_v_queue = dir_queue_v.ConsumeSync(dir_v);
    
            if(got_v_queue && !cmd_active){
                cmd_active = true;
                std::string msg = "[MOTOR] vision send rotate ";
                msg+=std::to_string(dir_v);
                Logger::instance().info("motor", msg);
                cmd="rotate ";
                cmd+=std::to_string(dir_v)+"\n";
                last_cmd = cmd;
                g_serial.Write(cmd.c_str(),cmd.size());
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
        
    dir_queue_g.Finish();
    dir_queue_v.Finish();
    point_queue.Finish();
}
        /*
        if(g_serial.ReadLine(linebuf, sizeof(linebuf))){
            if(linebuf == "cmd_done\n"){
                cmd_active=false;
            }
        } //serial.readline( ) == "행동종료문구" >> cmd_active=false;
        
        if (point_queue.ConsumeSync(pnt) && !cmd_active) {
            //장애물 위치(각도)와 거리 파악
            cmd_active = true;
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
                //장애물 코드가 너무 자주 도는 것 방지
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                
                continue;
            }
        }
        // 2순위 네비게이션 방향 처리
        else if (dir_queue_g.ConsumeSync(dir_g) && 
                !point_queue.ConsumeSync(pnt) && 
                !cmd_active){
            
            cmd_active = true;
            if(dir_g==0.0f)
            {
                Logger::instance().info("motor", "[MOTOR] send straight");
                cmd="straight\n";
            }
            else
            {
                cmd="rotate ";
                std::string msg = "[MOTOR] gps send rotate ";
                msg+=dir_g;
                Logger::instance().info("motor", msg);
                cmd+=std::to_string(dir_g)+"\n";
            }
            g_serial.Write(cmd.c_str(), cmd.size());
        }

        // 3순위 비전 방향 처리
        else if(dir_queue_v.ConsumeSync(dir_v) &&
                !point_queue.ConsumeSync(pnt) &&
                !cmd_active){
            cmd_active = true;
            std::string msg = "[MOTOR] vision send rotate ";
            msg+=dir_v;
            Logger::instance().info("motor", msg);
            cmd="rotate ";
            cmd+=std::to_string(dir_v)+"\n";
            g_serial.Write(cmd.c_str(),cmd.size());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    */
    
