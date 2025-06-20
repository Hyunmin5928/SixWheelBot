#pragma once

#include <atomic>
#include <cstdint>
#include "../SafeQueue.hpp"
#include "../logger.h"

namespace IMU {

// 외부에서 정의하는 실행 플래그
extern std::atomic<bool> running;

// START / STOP 명령
enum Command : int {
    IMU_CMD_STOP  = 0,
    IMU_CMD_START = 1
};

// 샘플 주기·임계값
static constexpr uint16_t INTERVAL_MS  = 20;    // 20ms → 50Hz
static constexpr float    THRESH_ROLL  = 2.0f;  // Dead-band 각도
static constexpr float    THRESH_PITCH = 2.0f;
static constexpr int      SERVO_LIMIT  = 75;    // ±75°

// 센서 데이터 구조체
struct Data {
    float    roll;      // Roll (deg)
    float    pitch;     // Pitch (deg)
    float    yaw;       // Yaw (deg)
    uint64_t timestamp; // millis() 기준
};

/**
 * readerThread:
 *   - cmd_q에서 IMU_CMD_START 대기
 *   - BNO055 초기화 및 영점 캘리브레이션
 *   - running==true & STOP 전까지
 *       매 INTERVAL_MS마다 센서 읽어 Data를 data_q에 Produce
 *   - STOP 수신 또는 running=false 시 data_q.Finish() 후 종료
 */
void readerThread(
    SafeQueue<Data>&    data_q,
    SafeQueue<Command>& cmd_q
);

} // namespace IMU
