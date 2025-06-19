#pragma once

#include <atomic>
#include <cstdint>
#include "../SafeQueue.hpp"
#include "../logger.h"

using util::Logger;
using util::LogLevel;

// 전역 실행 플래그
extern std::atomic<bool> running;

// IMU 샘플링 주기 및 임계값
static constexpr uint16_t IMU_INTERVAL_MS    = 20;
static constexpr float    THRESHOLD_ROLL_DEG  = 2.0f;
static constexpr float    THRESHOLD_PITCH_DEG = 2.0f;
static constexpr int      SERVO_LIMIT_DEG     = 75;

// 자이로 데이터 타입
struct GyroData {
    float      roll;       // Roll (deg)
    float      pitch;      // Pitch (deg)
    float      yaw;        // Yaw (deg)
    uint64_t   timestamp;  // millis 기준
};

/**
 * gyro_reader_thread:
 *   - BNO055 초기화 및 영점 캘리브레이션
 *   - running == true 동안 매 IMU_INTERVAL_MS마다
 *     Euler 각도를 읽어 GyroData로 변환 후 gyro_q에 Produce
 *   - 종료 시 gyro_q.Finish()
 */
void gyro_reader_thread(SafeQueue<GyroData>& gyro_q);