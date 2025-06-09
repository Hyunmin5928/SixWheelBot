#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <Servo.h>
#include <SoftwareSerial.h>

/*==============================================================================
 1) 설정 파라미터
==============================================================================*/

// — IMU 샘플링 주기 (ms 단위, 여기선 20ms → 50Hz) —
static const uint16_t IMU_INTERVAL_MS = 20;

// — BNO055 I²C 주소 (스캐너로 확인한 0x28 또는 0x29) —
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x29);

// — GPS (SoftwareSerial) 핀 —
SoftwareSerial gpsSerial(11 /*RX*/, 12 /*TX*/);

// — 서보 핀 & 가동 범위 —
static const uint8_t PIN_SERVO_ROLL  =  9;   // Roll 모터
static const uint8_t PIN_SERVO_PITCH = 10;   // Pitch 모터
static const int     SERVO_LIMIT_DEG  = 75;  // ±75°

// — PID 파라미터 (Roll/Pitch 공통 예시) —
float kp = 2.0;
float ki = 0.5;
float kd = 0.1;

/*==============================================================================
 2) 전역 상태 변수
==============================================================================*/

Servo servoRoll, servoPitch;

// 캘리브레이션 오프셋 (출발 시 0으로 맞출 각)
float offRoll  = 0;
float offPitch = 0;

// PID 내부 변수
float  iAccRoll  = 0,  prevErrRoll  = 0;
float  iAccPitch = 0,  prevErrPitch = 0;

// 타이밍
unsigned long lastIMU = 0;

--------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600);

  // === 1) BNO055 초기화 ===
  if (!bno.begin(OPERATION_MODE_NDOF)) {
    Serial.println("ERROR: BNO055 not found");
    while (1);
  }
  delay(1000);
  bno.setExtCrystalUse(true);

  // === 2) 서보 초기화 ===
  // attach(pin, 최소_pulse, 최대_pulse)
  servoRoll.attach(PIN_SERVO_ROLL, 500, 2500);
  servoPitch.attach(PIN_SERVO_PITCH, 500, 2500);
  // 중립(90°) 위치로 이동
  servoRoll.write(90);
  servoPitch.write(90);
  delay(500);

  // === 3) 영점 캘리브레이션 ===
  calibrateZero();
  Serial.println("=== 캘리브레이션 완료 ===");
}

--------------------------------------------------------------------------------
void loop() {
  // — GPS NMEA 데이터 포워딩 —
  while (gpsSerial.available()) {
    Serial.write(gpsSerial.read());
  }

  // — IMU 샘플링 주기 체크 —
  unsigned long now = millis();
  if (now - lastIMU < IMU_INTERVAL_MS) return;
  lastIMU = now;

  // 1) Euler 각도 읽기
  imu::Vector<3> e = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  float yaw   = e.x();           // heading (unused)
  float roll  = e.y() - offRoll; // 영점 보정
  float pitch = e.z() - offPitch;

  // 2) PID 제어 계산 (목표 = 0°)
  float uRoll  = pidControl( roll,  prevErrRoll,  iAccRoll);
  float uPitch = pidControl( pitch, prevErrPitch, iAccPitch);

  // 3) 서보 각도로 맵핑 & 구동
  updateServo(servoRoll,  uRoll);
  updateServo(servoPitch, uPitch);

  // 4) 디버그 출력
  Serial.print("[CTRL] roll:");  Serial.print(roll,  2);
  Serial.print("  pitch:");      Serial.print(pitch, 2);
  Serial.print("  uRoll:");      Serial.print(uRoll, 2);
  Serial.print("  uPitch:");     Serial.println(uPitch,2);
}

--------------------------------------------------------------------------------
// 영점 캘리브레이션: 50번 연속 샘플 평균(약 1초)
void calibrateZero() {
  const int N = 50;
  float sumR=0, sumP=0;
  Serial.println("== 수평에 센서 올려놓고 2초 대기 ==");
  delay(2000);
  for (int i = 0; i < N; i++) {
    imu::Vector<3> e = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
    sumR += e.y();
    sumP += e.z();
    delay(20);
  }
  offRoll  = sumR / N;
  offPitch = sumP / N;
  Serial.print("offRoll=");  Serial.print(offRoll,2);
  Serial.print("  offPitch="); Serial.println(offPitch,2);
}

// 단일 채널 PID 제어 (목표 0)
float pidControl(float measured,
                 float &prevErr, float &iAcc) {
  float err = 0 - measured;
  iAcc += err * (IMU_INTERVAL_MS/1000.0);
  float deriv = (err - prevErr)/(IMU_INTERVAL_MS/1000.0);
  prevErr = err;
  float u = kp*err + ki*iAcc + kd*deriv;
  // ±SERVO_LIMIT_DEG 으로 클램핑
  if      (u >  SERVO_LIMIT_DEG) u =  SERVO_LIMIT_DEG;
  else if (u < -SERVO_LIMIT_DEG) u = -SERVO_LIMIT_DEG;
  return u;
}

// u (–75°…+75°) → 서보(0…180) 매핑 & 쓰기
void updateServo(Servo &sv, float u) {
  int angle = constrain( 90 + (int)u,
                         90 - SERVO_LIMIT_DEG,
                         90 + SERVO_LIMIT_DEG );
  sv.write(angle);
}
