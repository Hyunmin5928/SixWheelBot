#include "imu_module.h"

// Arduino/Adafruit 헤더 (빌드 환경에 맞게 -I 옵션 추가 필요)
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <Servo.h>

#include <thread>
#include <sstream>
#include <cmath>
#include "../SafeQueue.hpp"
#include "../logger.h"

using namespace IMU;
static Adafruit_BNO055 bno(55, 0x29);
static Servo servoRoll, servoPitch;

// 영점 캘리브레이션 오프셋 및 PID 내부 상태
static float offR=0, offP=0;
static float iAccR=0, prevErrR=0, iAccP=0, prevErrP=0;
static constexpr float kp=2.0f, ki=0.5f, kd=0.1f;

// 1) 영점 캘리브레이션
static void calibrateZero() {
    constexpr int N=50;
    Logger::instance().info("imu","[IMU] Zero-calibration, place flat");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    float sumR=0, sumP=0;
    for(int i=0;i<N;i++){
        auto e = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
        sumR += e.y(); sumP += e.z();
        std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL_MS));
    }
    offR = sumR / N;
    offP = sumP / N;
    std::ostringstream os;
    os << "[IMU] Offsets → R=" << offR << ", P=" << offP;
    Logger::instance().info("imu", os.str());
}

// 2) PID 제어
static float pidControl(float err, float &prevErr, float &acc) {
    acc += err * (INTERVAL_MS / 1000.0f);
    float deriv = (err - prevErr) / (INTERVAL_MS / 1000.0f);
    prevErr = err;
    float u = kp * err + ki * acc + kd * deriv;
    return std::clamp(u, -static_cast<float>(SERVO_LIMIT), static_cast<float>(SERVO_LIMIT));
}

// 3) 서보 업데이트
static void updateServo(Servo &sv, float u) {
    int angle = std::clamp(90 + static_cast<int>(u),
                           90 - SERVO_LIMIT,
                           90 + SERVO_LIMIT);
    sv.write(angle);
}

void IMU::readerThread(
    SafeQueue<Data>&    imu_queue,
    SafeQueue<Command>& imu_cmd_queue
) {
    Logger::instance().info("imu","[IMU] Thread start, waiting START");
    Command cmd;
    if (!imu_cmd_queue.ConsumeSync(cmd) || cmd != Command::START) {
        Logger::instance().warn("imu","[IMU] No START, exiting");
        imu_queue.Finish();
        return;
    }
    Logger::instance().info("imu","[IMU] START received");

    // 센서 초기화
    if (!bno.begin(Adafruit_BNO055::OPERATION_MODE_NDOF)) {
        Logger::instance().error("imu","BNO055 not found");
        data_q.Finish();
        return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    bno.setExtCrystalUse(true);

    // 서보 초기화
    servoRoll.attach(9,500,2500);
    servoPitch.attach(10,500,2500);
    servoRoll.write(90);
    servoPitch.write(90);

    // 영점 캘리브레이션
    calibrateZero();

    // 주기 루프
    while (running.load()) {
        // STOP 명령 체크
        if (imu_cmd_queue.Pop(cmd) && cmd == Command::STOP) {
            Logger::instance().info("imu","[IMU] STOP received");
            break;
        }

        auto e = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
        float r = e.y() - offR;
        float p = e.z() - offP;
        float y = e.x();
        if (std::fabs(r) < THRESH_ROLL) r = 0;
        if (std::fabs(p) < THRESH_PITCH) p = 0;
        float ur = pidControl(-r, prevErrR, iAccR);
        float up = pidControl(-p, prevErrP, iAccP);
        updateServo(servoRoll, ur);
        updateServo(servoPitch, up);

        Data d{r, p, y, static_cast<uint64_t>(millis())};
        imu_queue.Produce(d);
        std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL_MS));
    }

    Logger::instance().info("imu","[IMU] Thread stopping");
    imu_queue.Finish();
}
