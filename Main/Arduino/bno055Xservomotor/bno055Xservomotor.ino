#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include <WString.h>

/*==============================================================================
 1) 설정 파라미터
==============================================================================*/

// BTS7960, 서보모터 핀 정의
static const int L_L_EN_PIN  = 2;
static const int L_R_EN_PIN  = 4;
static const int L_L_PWM_PIN = 3;  // forward
static const int L_R_PWM_PIN = 5;  // backward

static const int R_L_EN_PIN  = 7; 
static const int R_R_EN_PIN  = 8;
static const int R_L_PWM_PIN = 6;  // forward
static const int R_R_PWM_PIN = 11;

static const uint8_t PIN_SERVO_PITCH  = 10;
static const uint8_t PIN_SERVO_LOCK   = 12;
static const int     LOCKED_ANGLE     = 0;
static const int     UNLOCKED_ANGLE   = 90; 

// IMU 샘플링 주기 (ms 단위, 20ms → 50Hz)
static const uint16_t IMU_INTERVAL_MS      = 20;

// 작은 흔들림(±°) 범위: 이내에서는 서보 동작 억제 (Dead-band)
static const float      THRESHOLD_ROLL_DEG  = 2.0;
static const float      THRESHOLD_PITCH_DEG = 2.0;

float max_dist = 800.0f;

// BNO055 I²C 주소
Adafruit_BNO055 bno(55, 0x29);

// 서보 핀 & 가동 범위 (±75°)
static const int     SERVO_LIMIT_DEG  = 75;

// PID 파라미터 (Roll/Pitch 공통)
float kp = 2.0, ki = 0.5, kd = 0.1;

/*==============================================================================
 2) 전역 상태 변수
==============================================================================*/

Servo servoRoll, servoPitch;
Servo servoLock;

// 영점 캘리브레이션 오프셋
float offRoll  = 0, offPitch = 0, offYaw = 0;

// PID 내부 상태
float iAccRoll  = 0, prevErrRoll  = 0;
float iAccPitch = 0, prevErrPitch = 0;

// IMU 타이밍
unsigned long lastIMU = 0;

// 기본 PWM 세기 (0~255)
const int DEFAULT_PWM = 100;

float targetAngle = NAN;   // NaN 이면 회전 모드 아님
const float ANGLE_TOLERANCE = 2.0f;

void set_motor_on();
void set_motor_off();
void set_pwm_zero();
void driveStraight();
void driveStop();
void driveBack();
void rotateToAngle(float targetAngle);
void calibrateZero();
float pidControl(float measured, float &prevErr, float &iAcc);
void updateServo(Servo &sv, float u);

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
  servoPitch.attach(PIN_SERVO_PITCH, 500, 2500);
  servoPitch.write(90);
  delay(500);

  // 3) 잠금 서보 초기화 
  servoLock.attach(PIN_SERVO_LOCK, 500, 2500);
  servoLock.write(UNLOCKED_ANGLE);

  // 4) 영점 캘리브레이션
  calibrateZero();
  // Serial.println("=== 캘리브레이션 완료 ===");

  // 5) 모터 드라이버 핀 출력 설정
  pinMode(L_L_EN_PIN,  OUTPUT);
  pinMode(L_R_EN_PIN,  OUTPUT);
  pinMode(L_L_PWM_PIN, OUTPUT);
  pinMode(L_R_PWM_PIN, OUTPUT);
  pinMode(R_L_EN_PIN,  OUTPUT);
  pinMode(R_R_EN_PIN,  OUTPUT);
  pinMode(R_L_PWM_PIN, OUTPUT);
  pinMode(R_R_PWM_PIN, OUTPUT);
  Serial.println("setup");
}

// 모터 시작
void set_motor_on(){
  digitalWrite(L_L_EN_PIN, HIGH);
  digitalWrite(L_R_EN_PIN, HIGH);
  digitalWrite(R_L_EN_PIN, HIGH);
  digitalWrite(R_R_EN_PIN, HIGH);
}

// 모터 종료
void set_motor_off(){
  digitalWrite(L_L_EN_PIN, LOW);
  digitalWrite(L_R_EN_PIN, LOW);
  digitalWrite(R_L_EN_PIN, LOW);
  digitalWrite(R_R_EN_PIN, LOW);
}

void set_pwm_zero(){
  analogWrite(L_L_PWM_PIN, 0);
  analogWrite(L_R_PWM_PIN, 0);
  analogWrite(R_L_PWM_PIN, 0);
  analogWrite(R_R_PWM_PIN, 0);
}

// 전진
void driveStraight() {
  set_motor_on();
  set_pwm_zero();
  analogWrite(L_L_PWM_PIN, 0);
  analogWrite(L_R_PWM_PIN, DEFAULT_PWM);
  analogWrite(R_L_PWM_PIN, DEFAULT_PWM);
  analogWrite(R_R_PWM_PIN, 0);
  servoLock.write(LOCKED_ANGLE);
  Serial.println(">> straight");
}

// 정지
void driveStop() {
  set_pwm_zero();
  set_motor_off();
  servoLock.write(UNLOCKED_ANGLE);
  Serial.println(">> stop");
}

// 후진
void driveBack() {
  set_motor_on();
  set_pwm_zero();
  analogWrite(L_L_PWM_PIN, DEFAULT_PWM);
  analogWrite(L_R_PWM_PIN, 0);
  analogWrite(R_L_PWM_PIN, 0);
  analogWrite(R_R_PWM_PIN, DEFAULT_PWM);
  Serial.println(">> back");
}

float normalizeAngle(float angle){
  angle = fmod(angle + 180.0, 360.0);
  if(angle <0) angle += 360.0;
  return angle - 180.0;
}

//  회전: 현재 yaw와 목표 yaw 비교하며 회전
void rotateToAngle(float targetAngle) {
  Serial.print(">> rotate to ");
  Serial.println(targetAngle);
  set_motor_on();
  float targetYaw = bno.getVector(Adafruit_BNO055::VECTOR_EULER).x() + targetAngle;
  if(targetYaw >= 360.0f) targetYaw -=360.0f;
  if(targetYaw < 0.0f) targetYaw +=360.0f;
  bool reached = false;
  int count =0;

  // 회전 방향에 따라 모터 반대회전
  if (targetAngle > 0) {
      // 오른쪽 회전: 왼쪽 전진, 오른쪽 후진
      analogWrite(L_L_PWM_PIN, 0);
      analogWrite(L_R_PWM_PIN, DEFAULT_PWM);
      analogWrite(R_L_PWM_PIN, 0);
      analogWrite(R_R_PWM_PIN, DEFAULT_PWM);
  } else {
      // 왼쪽 회전: 왼쪽 후진,     오른쪽 전진
      analogWrite(L_L_PWM_PIN, DEFAULT_PWM);
      analogWrite(L_R_PWM_PIN, 0);
      analogWrite(R_L_PWM_PIN, DEFAULT_PWM);
      analogWrite(R_R_PWM_PIN, 0);
  }

  while (!reached) {
    imu::Vector<3> e = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
    float currYaw = e.x();  // 0~360 범위
    float err = normalizeAngle(targetYaw - currYaw);
    // 오차 계산 (−180~180)
    
    // 목표 도달 시 정지
    if (fabs(err) < ANGLE_TOLERANCE) {
      reached = true;
      driveStop();
      Serial.println(">> reached");
      break;
    }

    if( count %15 ==0){
      Serial.print("cur : ");
      Serial.print(currYaw);
      Serial.print(" err : ");
      Serial.println(err);
    }
    
    count ++;
    
    
    delay(20);
  }
}

void calibrateZero() {
  const int N = 50;
  float sumR = 0, sumP = 0;
  // Serial.println("== 수평에 센서 올려놓고 3초 대기 ==");
  delay(3000);
  for (int i = 0; i < N; i++) {
    imu::Vector<3> e = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
    sumR += e.y();
    sumP += e.z();
    delay(IMU_INTERVAL_MS);
  }
  offRoll  = sumR / N;
  offPitch = sumP / N;
  // Serial.print("offRoll=");  Serial.print(offRoll, 2);
  // Serial.print("  offPitch="); Serial.println(offPitch, 2);
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

void loop() {
  // IMU 기반 Pitch 서보 제어 (Serial.available() 밖)
  unsigned long now = millis();
  if (now - lastIMU >= IMU_INTERVAL_MS) {
    lastIMU = now;
    imu::Vector<3> e = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
    float currPitch = e.z() - offPitch;  // Pitch 읽기: e.z()
    float uPitch = pidControl(currPitch, prevErrPitch, iAccPitch);
    updateServo(servoPitch, uPitch);
  }

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    Serial.print("cmd receive : ");
    Serial.println(cmd);
    if      (cmd == "straight") { driveStraight();  targetAngle = NAN; }
    else if (cmd == "stop")     { driveStop();      targetAngle = NAN; }
    else if (cmd == "back")     { driveBack();      targetAngle = NAN; }
    else {
      String sep = cmd.substring(0, cmd.indexOf(' '));
      String info = cmd.substring(sep.length() + 1);
      info.trim();
      if (sep == "rotate") {
        targetAngle = info.toFloat();
        rotateToAngle(targetAngle);
        Serial.println("cmd_done");
      }
      else if (sep == "avoid") {
        //token[0] : 장애물과의 각도
        //token[1] : 장애물과의 거리
        String token[2];
        int tokenCount = 0;
        while (tokenCount < 2) {
          int spaceIndex = info.indexOf(' ');
          token[tokenCount] = info.substring(0, spaceIndex);
          info = info.substring(spaceIndex + 1);
          info.trim();
          tokenCount++;
        }
        String log = "avoid sep : " + token[0] + ", " + token[1];
        Serial.println(log);
        float angle = token[0].toFloat();
        float dist = token[1].toFloat();

        float w_dist = 1.0f - dist / max_dist;  // 거리가 멀수록 가중치는 작아짐 (틀어지는 각도 감소)
        if (w_dist < 0.3f) w_dist = 0.3f;       // 너무 작아지지 않도록 조정
        float w_ang = 1.0f - fabs(angle) / 120.0f;    // 각도가 0에서 멀수록 가중치는 작아짐 (틀어지는 각도 감소)
        if (w_ang < 0.3f) w_ang = 0.3f; // 가중치가 너무 작아지지 않도록 조정
        float weight = (w_dist + w_ang)/2.0f;
        targetAngle *= weight;

        rotateToAngle(targetAngle);
        driveStraight();
        delay(1500);
        rotateToAngle(-targetAngle);
        Serial.println("cmd_done");
      }
    }
  }

  // 원본 IMU 제어 블록 (주석 유지)
  // unsigned long now = millis();
  // if (now - lastIMU >= IMU_INTERVAL_MS) {
  //   lastIMU = now;
  //   imu::Vector<3> e = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  //   float currYaw   = e.x() - offYaw;
  //   float currPitch = e.y() - offPitch;
  //   float uPitch = pidControl(currPitch, prevErrPitch, iAccPitch);
  //   updateServo(servoPitch, uPitch);
  //   if (!isnan(targetAngle)) {
  //     float err = targetAngle - currYaw;
  //     if (err > 180)  err -= 360;
  //     if (err < -180) err += 360;
  //     if (fabs(err) < ANGLE_TOLERANCE) {
  //       driveStop();
  //       targetAngle = NAN;
  //     } else if (err > 0) {
  //       analogWrite(L_L_PWM_PIN, DEFAULT_PWM);
  //       analogWrite(L_R_PWM_PIN, 0);
  //       analogWrite(R_L_PWM_PIN, 0);
  //       analogWrite(R_R_PWM_PIN, DEFAULT_PWM);
  //     } else {
  //       analogWrite(L_L_PWM_PIN, 0);
  //       analogWrite(L_R_PWM_PIN, DEFAULT_PWM);
  //       analogWrite(R_L_PWM_PIN, DEFAULT_PWM);
  //       analogWrite(R_R_PWM_PIN, 0);
  //     }
  //   }
  // }

  delay(10); // loop 주기
}
