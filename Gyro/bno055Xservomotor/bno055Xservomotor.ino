#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <Servo.h>
#include <math.h>  // fabs 사용을 위해 추가

/*==============================================================================
 1) 설정 파라미터
==============================================================================*/

// IMU 샘플링 주기 (20 ms → 50 Hz)
static const uint16_t IMU_INTERVAL_MS = 20;

// BNO055 I²C 주소 (스캐너로 확인한 0x28 또는 0x29)
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x29);

// 서보 핀 & 가동 범위 (±75°)
static const uint8_t PIN_SERVO_ROLL  =  9;
static const uint8_t PIN_SERVO_PITCH = 10;
static const int     SERVO_LIMIT_DEG  = 75;

// 동작 임계치 (±10° 이내는 무시)
static const float ACTIVATE_THRESHOLD_DEG = 10.0;

// PID 파라미터 (Roll/Pitch 공통 예시)
float kp = 2.0, ki = 0.5, kd = 0.1;

/*==============================================================================
 2) 전역 상태 변수
==============================================================================*/

Servo servoRoll, servoPitch;

// 동작 제어 플래그 (기본 false)
bool flag = false;

// 영점 캘리브레이션 오프셋
float offRoll  = 0, offPitch = 0;

// PID 내부 상태
float iAccRoll   = 0, prevErrRoll  = 0;
float iAccPitch  = 0, prevErrPitch = 0;

// IMU 타이밍
unsigned long lastIMU = 0;

void setup() {
  Serial.begin(115200);

  // 1) BNO055 초기화
  if (!bno.begin(OPERATION_MODE_NDOF)) {
    Serial.println("ERROR: BNO055 not found");
    while (1);
  }
  delay(1000);
  bno.setExtCrystalUse(true);

  // 2) 서보 초기화
  servoRoll.attach(PIN_SERVO_ROLL, 500, 2500);
  servoPitch.attach(PIN_SERVO_PITCH, 500, 2500);
  servoRoll.write(90);
  servoPitch.write(90);
  delay(500);

  // 3) 영점 캘리브레이션
  calibrateZero();
  Serial.println("=== 캘리브레이션 완료 ===");
}

void loop() {
  // 0) 시리얼 명령 처리
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "start") {
      flag = true;
      Serial.println(">> START received");
    }
    else if (cmd == "stop") {
      flag = false;
      Serial.println(">> STOP received");
    }
  }

  // 50 Hz IMU 처리 타이밍
  unsigned long now = millis();
  if (now - lastIMU < IMU_INTERVAL_MS) return;
  lastIMU = now;

  if (flag) {
    // === 동작 모드 ===

    // Euler 각도 읽기
    imu::Vector<3> e = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
    float roll_raw  = e.y();
    float pitch_raw = e.z();
    float yaw       = e.x();

    // 보정된 Roll/Pitch 계산
    float roll  = roll_raw  - offRoll;
    float pitch = pitch_raw - offPitch;

    // Roll: |roll| ≥ threshold일 때만 동작, 그 외 중립
    if (fabs(roll) >= ACTIVATE_THRESHOLD_DEG) {
      float uRoll = pidControl(roll, prevErrRoll, iAccRoll);
      updateServo(servoRoll, -uRoll);  // 반전 적용
    } else {
      servoRoll.write(90);
    }

    // Pitch: |pitch| ≥ threshold일 때만 동작, 그 외 중립
    if (fabs(pitch) >= ACTIVATE_THRESHOLD_DEG) {
      float uPitch = pidControl(pitch, prevErrPitch, iAccPitch);
      updateServo(servoPitch, uPitch);
    } else {
      servoPitch.write(90);
    }

    // 3) ROLL, PITCH, YAW 한 줄로 출력
    Serial.print("ROLL = ");  Serial.print(roll, 2);
    Serial.print(", PITCH = "); Serial.print(pitch, 2);
    Serial.print(", YAW = ");   Serial.println(yaw, 2);
  }
  else {
    // === 중지 모드 ===
    servoRoll.write(90);
    servoPitch.write(90);
    Serial.println("Not Running");
  }
}

//==============================================================================
// 영점 캘리브레이션: 50 샘플 평균 (약 1초)
//==============================================================================
void calibrateZero() {
  const int N = 50;
  float sumR = 0, sumP = 0;
  Serial.println("== 수평에 센서 올려놓고 2초 대기 ==");
  delay(2000);
  for (int i = 0; i < N; i++) {
    imu::Vector<3> e = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
    sumR += e.y();
    sumP += e.z();
    delay(IMU_INTERVAL_MS);
  }
  offRoll  = sumR / N;
  offPitch = sumP / N;
  Serial.print("offRoll=");  Serial.print(offRoll, 2);
  Serial.print("  offPitch="); Serial.println(offPitch, 2);
}

//==============================================================================
// 단일 채널 PID 제어 (목표 = 0°)
//==============================================================================
float pidControl(float measured, float &prevErr, float &iAcc) {
  float err    = -measured;
  iAcc        += err * (IMU_INTERVAL_MS / 1000.0);
  float deriv  = (err - prevErr) / (IMU_INTERVAL_MS / 1000.0);
  prevErr      = err;
  float u      = kp * err + ki * iAcc + kd * deriv;
  // ±SERVO_LIMIT_DEG 클램핑
  if      (u >  SERVO_LIMIT_DEG) u =  SERVO_LIMIT_DEG;
  else if (u < -SERVO_LIMIT_DEG) u = -SERVO_LIMIT_DEG;
  return u;
}

//==============================================================================
// u(–75°…+75°) → 서보 각도(0…180) 매핑 & 쓰기
//==============================================================================
void updateServo(Servo &sv, float u) {
  int angle = constrain(90 + (int)u,
                        90 - SERVO_LIMIT_DEG,
                        90 + SERVO_LIMIT_DEG);
  sv.write(angle);
}
