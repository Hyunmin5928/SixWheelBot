#include "motorCtrl.h"
#define RPWM 1
#define LPWM 23
#define LR_EN 4
#define LL_EN 5
#define RR_EN 29
#define RL_EN 31

#define maxPulse 255

//@brief pwm값 유효성 확인
#define validate_pwm(pwm)                         \
    if (!pwm_isvalid(pwm)) {                            \
        std::cout << "pwm value is invaild\n";       \
        return;                                                      \
    }
// R, L MOtor 하나의 클래스에 넣어서 조져야함
// R, L Motor in one class


// private
#pragma region Private functions
void Motor::motor_setup(int r_pwmPin, int l_pwmPin, int rr_enPin, int rl_enPin, int lr_enPin, int ll_enPin) {
    wiringPiSetup();
    pinMode(r_pwmPin, OUTPUT);
    pinMode(l_pwmPin, OUTPUT);
    pinMode(rr_enPin, OUTPUT);
    pinMode(rl_enPin, OUTPUT);
    pinMode(lr_enPin, OUTPUT);
    pinMode(ll_enPin, OUTPUT);

    lmotor_direction_front();
    rmotor_direction_front();

    //R_Motor, L_Motor 0으로 초기화, 최대 펄스 255
    //R_Motor, L_Motor initialize 0, max purse 255
    softPwmCreate(r_pwmPin, 0, maxPulse);
    softPwmCreate(l_pwmPin, 0, maxPulse);
}

void Motor::motor_setup()
{
    motor_setup(RPWM, LPWM, RR_EN, RL_EN, LR_EN, LL_EN);
}

bool Motor::pwm_isvalid(int pwm)
{
    if(pwm > maxPulse || pwm < 0){
        return false;
    }else{
        return true;
    }
}

void Motor::lmotor_direction_front(bool front=true)
{
    if (front == true) {
        digitalWrite(LR_ENPin, HIGH);
        digitalWrite(LL_ENPin, LOW);
    }
    else {
        digitalWrite(LR_ENPin, HIGH);
        digitalWrite(LL_ENPin, LOW);
    }
}

void Motor::rmotor_direction_front(bool front=true)
{
    if (front == true) {
        digitalWrite(RR_ENPin, HIGH);
        digitalWrite(RL_ENPin, LOW);
    }
    else {
        digitalWrite(RR_ENPin, HIGH);
        digitalWrite(RL_ENPin, LOW);
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

Motor::Motor(int r_pwmPin, int l_pwmPin, int rr_enPin, int rl_enPin, int lr_enPin, int ll_enPin){
    
    motor_setup(int r_pwmPin, int l_pwmPin, int rr_enPin, int rl_enPin, int lr_enPin, int ll_enPin);
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
    softPwmWrite(R_pwmPin, pwm);
    softPwmWrite(L_pwmPin, pwm);
}

void Motor::backoff(int pwm)
{
    validate_pwm(pwm);
    rmotor_direction_front(false);
    lmotor_direction_front(false);
    softPwmWrite(R_pwmPin, pwm);
    softPwmWrite(L_pwmPin, pwm);
}

void Motor::rotate(int pwm, float degree)
{
    validate_pwm(pwm);
    if (degree > 0) 
    {
        rmotor_direction_front(false);
        lmotor_direction_front();
    }
    else {
        rmotor_direction_front();
        lmotor_direction_front(false);
    }
    // 모터 최대 150rpm, 설정 최대 pwm 255, 6.0f는 초당 각속도 계산을 위한 상수
    float dgrspeed = 150.0f / 255.0f * pwm * 6.0f;
    float delaytime = degree / dgrspeed;

    softPwmWrite(R_pwmPin, pwm);
    softPwmWrite(L_pwmPin, pwm);

    delay(delaytime);

}

// 커브 함수 동작 지침
// 1. degree값만큼 회전
// 2. 각 바퀴가 pwm값만큼 회전 (딜레이 시간 계산 필요)
// 3. -degree값만큼 회전

// l_pwm, r_pwm 말고, 원하는 pwm값 (오,왼 평균pwm), 각도를 설정해서 커브를 구현하는건 어떨까?


void Motor::left_curve(int pwm)
{

}

void Motor::right_curve(int pwm)
{
}

void Motor::left_curve(int l_pwm, int r_pwm)
{
}

void Motor::right_curve(int l_pwm, int r_pwm)
{
}

void Motor::curve(bool recover=false, int pwm, float degree)
{
    
}

#pragma endregion

int main() {
    Motor motor;
    motor.straight(100);
    delay(500);
    motor.backoff(50);
    delay(300);
    motor.rotate(50, 30);

    return 0;
}