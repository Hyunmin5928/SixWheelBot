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
static const uint16_t IMU_INTERVAL_MS = 20;

// BNO055 I²C 주소 (0x28 또는 0x29)
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x29);

// GPS (SoftwareSerial) 핀
SoftwareSerial gpsSerial(11 /*RX*/, 12 /*TX*/);

// 서보 핀 & 가동 범위 (±75°)
static const uint8_t PIN_SERVO_ROLL  =  9;
static const uint8_t PIN_SERVO_PITCH = 10;
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

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600);

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
  // GPS NMEA 데이터 실시간 출력
  while (gpsSerial.available()) {
    String nmea = gpsSerial.readStringUntil('\n');
    nmea.trim();
    if (nmea.length()) {
      Serial.print("GPS: ");
      Serial.println(nmea);
    }
  }

  // IMU 샘플링 주기 체크
  unsigned long now = millis();
  if (now - lastIMU < IMU_INTERVAL_MS) return;
  lastIMU = now;

  // Euler 읽기 & PID → 서보 제어
  imu::Vector<3> e = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  float roll  = e.y() - offRoll;   // 영점 보정된 Roll
  float pitch = e.z() - offPitch;  // 영점 보정된 Pitch

  // PID 제어 계산 (목표 = 0°)
  float uRoll  = pidControl(roll,  prevErrRoll,  iAccRoll);
  float uPitch = pidControl(pitch, prevErrPitch, iAccPitch);

  // 서보 구동
  updateServo(servoRoll,  -uRoll);
  updateServo(servoPitch, uPitch);

  // =================================================================

  // EKF용 센서 값 읽기
  imu::Vector<3> gyro   = bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);
  imu::Vector<3> linacc = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
  imu::Vector<3> mag    = bno.getVector(Adafruit_BNO055::VECTOR_MAGNETOMETER);
  float          temp   = bno.getTemp();

  // EKF용 시리얼 출력
  Serial.print("GYRO [dps]: ");
    Serial.print(gyro.x(), 2); Serial.print(", ");
    Serial.print(gyro.y(), 2); Serial.print(", ");
    Serial.println(gyro.z(), 2);

  Serial.print("ACCL [m/s²]: ");
    Serial.print(linacc.x(), 2); Serial.print(", ");
    Serial.print(linacc.y(), 2); Serial.print(", ");
    Serial.println(linacc.z(), 2);

  Serial.print("MAG  [µT]: ");
    Serial.print(mag.x(), 2); Serial.print(", ");
    Serial.print(mag.y(), 2); Serial.print(", ");
    Serial.println(mag.z(), 2);

  Serial.print("TEMP [°C]: ");
    Serial.println(temp, 1);

  Serial.println("-----------------------------------------------");  // 데이터 블록 구분 빈 줄

}

//==============================================================================
// 영점 캘리브레이션: 50샘플 평균 (약 1초)
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
  offRoll  = sumR  / N;
  offPitch = sumP / N;
  Serial.print("offRoll=");  Serial.print(offRoll, 2);
  Serial.print("  offPitch="); Serial.println(offPitch, 2);
}

//==============================================================================
// 단일 채널 PID 제어 (목표 = 0°)
//==============================================================================
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

//==============================================================================
// u (–75°…+75°) → 서보 각도(0…180) 매핑 & 쓰기
//==============================================================================
void updateServo(Servo &sv, float u) {
  int angle = constrain(90 + (int)u,
                        90 - SERVO_LIMIT_DEG,
                        90 + SERVO_LIMIT_DEG);
  sv.write(angle);
}
