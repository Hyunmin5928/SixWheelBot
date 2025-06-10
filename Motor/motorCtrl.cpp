#include "motorCtrl.h"
#include <wiringPi.h>
#include <softPwm.h>
#include <iostream>

#define L_RPWM 1
#define L_LPWM 23
#define R_RPWM 24
#define R_LPWM 26

#define L_REN 4
#define L_LEN 5
#define R_REN 6
#define R_LEN 10

#define maxPulse 255

//@brief pwm값 유효성 확인
#define validate_pwm(pwm)                         \
    if (!pwm_isvalid(pwm)) {                            \
        std::cout << "pwm value is invaild\n";       \
        return;                                                      \
    }

// private
#pragma region Private functions

void Motor::lmotor_direction_front(bool front=true)
{
    if (front == true) {
        digitalWrite(L_RENPin, HIGH);
        digitalWrite(L_LENPin, LOW);
    }
    else {
        digitalWrite(L_RENPin, HIGH);
        digitalWrite(L_LENPin, LOW);
    }
}

void Motor::rmotor_direction_front(bool front=true) 
{
    if (front == true) {
        digitalWrite(R_RENPin, HIGH);
        digitalWrite(R_LENPin, LOW);
    }
    else {
        digitalWrite(R_RENPin, HIGH);
        digitalWrite(R_LENPin, LOW);
    }
}

void Motor::motor_setup(int lr_pwmPin, int ll_pwmPin, int rr_pwmPin, int rl_pwmPin, int rrenPin, int rlenPin, int lrenPin, int llenPin) {
    wiringPiSetup();
    L_RpwmPin, L_LpwmPin, R_RpwmPin, R_LpwmPin, R_RENPin, R_LENPin, L_RENPin, L_LENPin = lr_pwmPin, ll_pwmPin, rr_pwmPin, rl_pwmPin, rrenPin, rlenPin, lrenPin, llenPin;
    pinMode(L_RpwmPin, OUTPUT);
    pinMode(L_LpwmPin, OUTPUT);
    pinMode(R_RpwmPin, OUTPUT);
    pinMode(R_LpwmPin, OUTPUT);
    pinMode(R_RENPin, OUTPUT);
    pinMode(R_LENPin, OUTPUT);
    pinMode(L_RENPin, OUTPUT);
    pinMode(L_LENPin, OUTPUT);

    lmotor_direction_front();
    rmotor_direction_front();

    //R_Motor, L_Motor 0으로 초기화, 최대 펄스 255
    //R_Motor, L_Motor initialize 0, max purse 255
    softPwmCreate(L_RpwmPin, 0, maxPulse);
    softPwmCreate(L_LpwmPin, 0, maxPulse);
    softPwmCreate(R_RpwmPin, 0, maxPulse);
    softPwmCreate(R_LpwmPin, 0, maxPulse);
}

void Motor::motor_setup()
{
    motor_setup(L_RPWM, L_LPWM, R_RPWM, R_LPWM, R_REN, R_LEN, L_REN, L_LEN);
}

bool Motor::pwm_isvalid(int pwm)
{
    if(pwm > maxPulse || pwm < 0){
        return false;
    }else{
        return true;
    }
}


#pragma endregion

// public
#pragma region Constructor functions
Motor::Motor()
{
    motor_setup();
    std::cout << "motor set up : default\n";
}

Motor::Motor(int lr_pwmPin, int ll_pwmPin,int rr_pwmPin, int rl_pwmPin, int rr_enPin, int rl_enPin, int lr_enPin, int ll_enPin){
    
    motor_setup(lr_pwmPin, ll_pwmPin, rr_pwmPin, rl_pwmPin, rr_enPin, rl_enPin, lr_enPin, ll_enPin);
    std::cout << "motor set up : user setting\n";
}

Motor::~Motor(){
    std::cout << "motor instance deleted\n";
}
#pragma endregion

#pragma region Move functions
void Motor::straight(int pwm)
{
    validate_pwm(pwm);
    rmotor_direction_front();
    lmotor_direction_front();
    softPwmWrite(R_RpwmPin, pwm); //positive forward
    softPwmWrite(R_LpwmPin, 0); // must turn off this pin when using R_pwmPin
    softPwmWrite(L_RpwmPin, pwm); //positive forward
    softPwmWrite(L_LpwmPin, 0); //must turn off this pin when using R_pwmPin
}

void Motor::backoff(int pwm)
{
    validate_pwm(pwm);
    rmotor_direction_front(false);
    lmotor_direction_front(false);
    softPwmWrite(R_LpwmPin, pwm);
    softPwmWrite(R_RpwmPin, 0);
    softPwmWrite(L_LpwmPin, pwm);
    softPwmWrite(L_RpwmPin, 0);
}

void Motor::stop(){
    softPwmWrite(L_RPWM, 0);
    softPwmWrite(R_RPWM, 0);
    softPwmWrite(L_LPWM, 0);
    softPwmWrite(R_LPWM, 0);
}

void Motor::rotate(int pwm, float degree)
{
    validate_pwm(pwm);

    float dgrspeed = 150.0f / 255.0f * pwm * 6.0f;
    float abs_dgr = degree;
    if(abs_dgr <0){
        abs_dgr *= -1.0f;
    }
    float delaytime = abs_dgr / dgrspeed;
    if (degree > 0) 
    {
        rmotor_direction_front(false);
        lmotor_direction_front();
        softPwmWrite(R_LpwmPin, pwm);
        softPwmWrite(R_RpwmPin,0);
        softPwmWrite(L_RpwmPin, pwm);
        softPwmWrite(L_LpwmPin, 0);
    }
    else {
        rmotor_direction_front();
        lmotor_direction_front(false);
        softPwmWrite(R_RpwmPin, pwm);
        softPwmWrite(R_LpwmPin,0);
        softPwmWrite(L_LpwmPin, pwm);
        softPwmWrite(L_RpwmPin, 0);
    }
    // 모터 최대 150rpm, 설정 최대 pwm 255, 6.0f는 초당 각속도 계산을 위한 상수

    delay(delaytime);

}

// 커브 함수 동작 지침
// 1. degree값만큼 회전
// 2. 각 바퀴가 pwm값만큼 회전 (딜레이 시간 계산 필요)
// 3. -degree값만큼 회전

// l_pwm, r_pwm 말고, 원하는 pwm값 (오,왼 평균pwm), 각도를 설정해서 커브를 구현하는건 어떨까?



void Motor::curve(int pwm, float degree, bool recover=false)
{
    
}

#pragma endregion

int main() {
    Motor motor;
    motor.straight(100);
    delay(300);
    motor.backoff(50);
    delay(200);
    motor.rotate(50, 30);

    return 0;
}