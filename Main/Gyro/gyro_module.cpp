#include "gyro_module.h"
#include <utility/imumaths.h>
#include <Servo.h>
#include <thread>

// BNO055 센서 객체
static Adafruit_BNO055 bno(55, 0x29);
// 서보 객체
static Servo servoRoll, servoPitch;
// 오프셋 및 PID 상태
static float offRoll = 0, offPitch = 0;
static float iAccRoll = 0, prevErrRoll = 0;
static float iAccPitch = 0, prevErrPitch = 0;
// PID 파라미터
static constexpr float kp = 2.0f, ki = 0.5f, kd = 0.1f;

// 영점 캘리브레이션 함수
static void calibrateZero() {
    constexpr int N = 50;
    Logger::instance().info("imu", "[gyro] Zero calibration start (place flat)");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    float sumR = 0, sumP = 0;
    for (int i = 0; i < N; ++i) {
        auto e = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
        sumR += e.y(); sumP += e.z();
        std::this_thread::sleep_for(std::chrono::milliseconds(IMU_INTERVAL_MS));
    }
    offRoll  = sumR / N;
    offPitch = sumP / N;
    std::ostringstream oss;
    oss << "[gyro] Offsets: roll=" << offRoll << ", pitch=" << offPitch;
    Logger::instance().info("imu", oss.str());
}

// PID 제어 및 서보 업데이트
static float pidControl(float err, float &prevErr, float &acc) {
    acc += err * (IMU_INTERVAL_MS / 1000.0f);
    float deriv = (err - prevErr) / (IMU_INTERVAL_MS / 1000.0f);
    prevErr = err;
    float u = kp * err + ki * acc + kd * deriv;
    return std::clamp(u, -static_cast<float>(SERVO_LIMIT_DEG), static_cast<float>(SERVO_LIMIT_DEG));
}
static void updateServo(Servo &sv, float u) {
    int angle = std::clamp(90 + static_cast<int>(u), 90 - SERVO_LIMIT_DEG, 90 + SERVO_LIMIT_DEG);
    sv.write(angle);
}

void gyro_reader_thread(SafeQueue<GyroData>& gyro_q) {
    Logger::instance().info("imu", "[gyro] Thread start");

    // 1) 센서 초기화
    if (!bno.begin(Adafruit_BNO055::OPERATION_MODE_NDOF)) {
        Logger::instance().error("imu", "BNO055 not found");
        gyro_q.Finish();
        return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    bno.setExtCrystalUse(true);

    // 2) 서보 초기화
    servoRoll.attach(9, 500, 2500);
    servoPitch.attach(10, 500, 2500);
    servoRoll.write(90);
    servoPitch.write(90);

    // 3) 영점 캘리브레이션
    calibrateZero();

    // 4) 읽기 루프
    while (running.load()) {
        auto e = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
        float r = e.y() - offRoll;
        float p = e.z() - offPitch;
        float y = e.x();
        // Dead-band
        if (std::fabs(r) < THRESHOLD_ROLL_DEG)  r = 0;
        if (std::fabs(p) < THRESHOLD_PITCH_DEG) p = 0;
        // PID 및 서보 제어
        float ur = pidControl(-r, prevErrRoll, iAccRoll);
        float up = pidControl(-p, prevErrPitch, iAccPitch);
        updateServo(servoRoll, ur);
        updateServo(servoPitch, up);
        
        // 큐에 데이터 푸시
        GyroData d{r, p, y, static_cast<uint64_t>(millis())};
        gyro_q.Produce(std::move(d));

        std::this_thread::sleep_for(std::chrono::milliseconds(IMU_INTERVAL_MS));
    }

    Logger::instance().info("imu", "[gyro] Thread stopping");
    gyro_q.Finish();
}