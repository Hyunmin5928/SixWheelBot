#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <Servo.h>
#include <SoftwareSerial.h>

/*==============================================================================
 1) 설정 파라미터
==============================================================================*/

// IMU 샘플링 주기 (ms 단위, 20ms → 50Hz)
static const uint16_t IMU_INTERVAL_MS      = 20;

// 작은 흔들림(±°) 범위: 이내에서는 서보 동작 억제 (Dead-band)
static const float      THRESHOLD_ROLL_DEG  = 2.0;
static const float      THRESHOLD_PITCH_DEG = 2.0;

// BNO055 I²C 주소
Adafruit_BNO055 bno(55, 0x29);

// 서보 핀 & 가동 범위 (±75°)
static const uint8_t PIN_SERVO_ROLL   =  9;
static const uint8_t PIN_SERVO_PITCH  = 10;
static const int     SERVO_LIMIT_DEG  = 75;

// PID 파라미터 (Roll/Pitch 공통)
float kp = 2.0, ki = 0.5, kd = 0.1;

/*==============================================================================
 2) 전역 상태 변수
==============================================================================*/

Servo servoRoll, servoPitch;

// 영점 캘리브레이션 오프셋
float offRoll  = 0, offPitch = 0;

// PID 내부 상태
float iAccRoll  = 0, prevErrRoll  = 0;
float iAccPitch = 0, prevErrPitch = 0;

// IMU 타이밍
unsigned long lastIMU = 0;

// 플래그
bool imuEnabled = false;

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
  // --- 1) RPi로부터 “start”/“stop” 명령 수신 및 플래그 제어 ---
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if      (cmd == "start") imuEnabled = true;
    else if (cmd == "stop")  imuEnabled = false;
  }
  // imuEnabled가 false면 루프 전체 스킵
  if (!imuEnabled) return;

  // --- 2) IMU 샘플링 주기 체크 ---
  unsigned long now = millis();
  if (now - lastIMU < IMU_INTERVAL_MS) return;
  lastIMU = now;

  // --- 3) Euler 읽기 & 영점 보정 ---
  imu::Vector<3> e = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  float rawYaw   = e.x();
  float rawRoll  = e.y();
  float rawPitch = e.z();

  float corrRoll  = rawRoll  - offRoll;
  float corrPitch = rawPitch - offPitch;
  float corrYaw   = rawYaw;  // yaw는 영점 보정 없음

  // Dead-band 적용
  if (fabs(corrRoll)  < THRESHOLD_ROLL_DEG)  corrRoll  = 0;
  if (fabs(corrPitch) < THRESHOLD_PITCH_DEG) corrPitch = 0;

  // --- 4) PID 제어 & 서보 업데이트 ---
  float uRoll  = pidControl(corrRoll,  prevErrRoll,  iAccRoll);
  float uPitch = pidControl(corrPitch, prevErrPitch, iAccPitch);
  updateServo(servoRoll,  -uRoll);
  updateServo(servoPitch, uPitch);

  // --- 5) 시리얼 출력 (원데이터 + 보정값) ---
  Serial.print("ROLL = ");   Serial.print(rawRoll,  2);
  Serial.print(", PITCH = "); Serial.print(rawPitch, 2);
  Serial.print(", YAW = ");   Serial.println(rawYaw,   2);

  delay(1000);
}

void calibrateZero() {
  const int N = 50;
  float sumR = 0, sumP = 0;
  Serial.println("== 수평에 센서 올려놓고 3초 대기 ==");
  delay(3000);
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

float pidControl(float measured, float &prevErr, float &iAcc) {
  float err = -measured;
  iAcc += err * (IMU_INTERVAL_MS / 1000.0);
  float deriv = (err - prevErr) / (IMU_INTERVAL_MS / 1000.0);
  prevErr = err;
  float u = kp * err + ki * iAcc + kd * deriv;
  if      (u >  SERVO_LIMIT_DEG) u =  SERVO_LIMIT_DEG;
  else if (u < -SERVO_LIMIT_DEG) u = -SERVO_LIMIT_DEG;
  return u;
}

void updateServo(Servo &sv, float u) {
  int angle = constrain(90 + (int)u,
                        90 - SERVO_LIMIT_DEG,
                        90 + SERVO_LIMIT_DEG);
  sv.write(angle);
}
