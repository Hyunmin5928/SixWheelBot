#include <Servo.h>

Servo servo1;
Servo servo2;    

const int SERVO1_PIN = 9;    
const int SERVO2_PIN = 10;   

// 서보 최대 회전 한계 (±75°)
const int ANGLE_LIMIT = 75;  

void setup() {
  Serial.begin(9600);
  servo1.attach(SERVO1_PIN, 500, 2500);
  servo2.attach(SERVO2_PIN, 500, 2500);
  Serial.println("두 개의 서보모터 ±75° 테스트 시작");
}

void loop() {
  int pwm;

  // 1) 0° → +75°
  for (int ang = 0; ang <= ANGLE_LIMIT; ang++) {
    pwm = ang + 90;  // –90~+90 → 0~180 매핑
    servo1.write(pwm);
    servo2.write(pwm);
    Serial.print("각도: "); Serial.print(ang); Serial.println("°");
    delay(20);       
  }

  // 2) +75° → 0°
  for (int ang = ANGLE_LIMIT; ang >= 0; ang--) {
    pwm = ang + 90;
    servo1.write(pwm);
    servo2.write(pwm);
    Serial.print("각도: "); Serial.print(ang); Serial.println("°");
    delay(20);
  }

  // 3) 0° → –75°
  for (int ang = 0; ang >= -ANGLE_LIMIT; ang--) {
    pwm = ang + 90;
    servo1.write(pwm);
    servo2.write(pwm);
    Serial.print("각도: "); Serial.print(ang); Serial.println("°");
    delay(20);
  }

  // 4) –75° → 0°
  for (int ang = -ANGLE_LIMIT; ang <= 0; ang++) {
    pwm = ang + 90;
    servo1.write(pwm);
    servo2.write(pwm);
    Serial.print("각도: "); Serial.print(ang); Serial.println("°");
    delay(20);
  }

  delay(500);
}
