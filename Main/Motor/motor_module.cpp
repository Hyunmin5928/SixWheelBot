#include "motor_module.h"
#include <pthread.h>

static constexpr const char cmd_straight[] = "straight\n";
static constexpr const char cmd_stop [] = "stop\n";
static constexpr const char cmd_backoff [] = "back\n";
static constexpr const char cmd_rotate [] = "rotate\n";

// main문 수정 필요

void motor_thread(
    const std::string& port,
    unsigned int baud,
    SafeQueue<float>&    dir_queue,
    SafeQueue<LaserPoint>& point_queue
) {
    pthread_setname_np(pthread_self(), "[THREAD]MOTOR_D");
    // Arduino Serial Port Init
    if (!g_serial.Open(port.c_str(), baud)) {
        std::ostringstream oss;
        oss << "[MOTOR] Failed to open " << port << "@" << baud;
        Logger::instance().error("motor", oss.str());
        return;
    }
    Logger::instance().info("motor", "[MOTOR] Serial port opened: " + port);
    Motor motor;
    // Logger::instance().info("motor", "[motor_module] Motor Thread start");
    int status=0;
    char linebuf[128]; // 혹시 모를 SerialRead 용
    float angle = 0.0f;
    while (running.load()) {
        if(run_motor.load()){
            // 직진인 경우
            g_serial.Write(cmd_straight, sizeof(cmd_straight) - 1);
            // 후진인 경우
            g_serial.Write(cmd_backoff, sizeof(cmd_backoff) - 1);
            // 정지인 경우
            g_serial.Write(cmd_stop, sizeof(cmd_stop) - 1);
            // 회전인 경우
            angle = 124.3;
            // cmd_rotate = std::to_string(angle) // 회전 값(float 형) string으로 변경
            // g_serial.Write(cmd_rotate, sizeof(cmd_rotate) - 1);
        }
        // 10Hz 루프
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 종료 시 모터 정지
    motor.stop();
    // 큐에 더 이상 값이 들어오지 않음을 알림;
    dir_queue.Finish();
    point_queue.Finish();
}