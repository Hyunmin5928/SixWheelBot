#include "motorCtrl.h"
#include <wiringPi.h>
#include <softPwm.h>
#include <iostream>
#include <cmath>
#include <vector>

#define PI 3.1415926 

#define L_RPWM 1
#define L_LPWM 23
#define R_RPWM 24
#define R_LPWM 26

#define L_REN 4
#define L_LEN 5
#define R_REN 6
#define R_LEN 10

#define maxPulse 100
#define maxSpeed 150.0f

#define avoidDistance 300.0f //cm

//degree 값을 180 ~ -180 으로 설정함. 0~360으로 나올 경우 변수 변경 필요 (실제 받아오는 값은 전방 부채꼴 각도이므로, 유의)

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
        digitalWrite(L_LENPin, HIGH);
    }
    else {
        digitalWrite(L_RENPin, HIGH);
        digitalWrite(L_LENPin, HIGH);
    }
}

void Motor::rmotor_direction_front(bool front=true) 
{
    if (front == true) {
        digitalWrite(R_RENPin, HIGH);
        digitalWrite(R_LENPin, HIGH);
    }
    else {
        digitalWrite(R_RENPin, HIGH);
        digitalWrite(R_LENPin, HIGH);
    }
}

float Motor::calculate_dgrspeed(int pwm)
{
    return maxSpeed / 100.0f * pwm * 6.0f;
}

float Motor::average_pwm(int l_pwm, int r_pwm)
{
    return l_pwm + r_pwm / 2.0f;
}

float Motor::calculate_tan(float degree)
{
    return tan(degree * PI /180);
}

void Motor::calculate_twin_pwm(int pwm, float degree, int* pwm1, int* pwm2)
{
    float ratio = calculate_tan(degree);
    //*pwm1 = 2 * pwm / (1 / ratio + 1);
    //*pwm2 = 2 * pwm - pwm1;
    *pwm1 = pwm * (1.0f - ratio);
    *pwm2 = pwm * (1.0f + ratio);
    // PWM 값이 너무 급격하게 변하지 않도록 해야함
}

void Motor::lmotor_run(int pwm, bool front=true)
{
    std::cout<<"Lmotor_run\n";
    validate_pwm(pwm);
    lmotor_direction_front(front);
    if (front) {
        softPwmWrite(L_RpwmPin, pwm); //positive forward
        softPwmWrite(L_LpwmPin, 0); // must turn off this pin when using L_pwmPin
    }
    else {
        softPwmWrite(L_RpwmPin, 0); //negative backward
        softPwmWrite(L_LpwmPin, pwm);
    }

}

void Motor::rmotor_run(int pwm, bool front=true)
{
    std::cout << "Rmotor_run\n";
    validate_pwm(pwm);
    rmotor_direction_front(front);
    if (front) {
        softPwmWrite(R_RpwmPin, pwm); //positive forward
        softPwmWrite(R_LpwmPin, 0); // must turn off this pin when using L_pwmPin
    }
    else {
        softPwmWrite(R_RpwmPin, 0); //negative backward
        softPwmWrite(R_LpwmPin, pwm);
    }
}

void Motor::motor_setup(int lr_pwmPin, int ll_pwmPin, int rr_pwmPin, int rl_pwmPin, int rrenPin, int rlenPin, int lrenPin, int llenPin) {
    wiringPiSetup();
    std::cout<<"setup\n";
    L_RpwmPin = lr_pwmPin;
    L_LpwmPin = ll_pwmPin;
    R_RpwmPin = rr_pwmPin;
    R_LpwmPin = rl_pwmPin;
    R_RENPin = rrenPin;
    R_LENPin = rlenPin;
    L_RENPin = lrenPin;
    L_LENPin = llenPin;;

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

    //R_Motor, L_Motor 0으로 초기화, 최대 펄스 100
    //R_Motor, L_Motor initialize 0, max purse 100
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
    delay(1000);
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
    softPwmWrite(R_LpwmPin, 0);
    softPwmWrite(R_RpwmPin, 0);
    softPwmWrite(L_LpwmPin, 0);
    softPwmWrite(L_RpwmPin, 0);
    digitalWrite(R_LENPin, LOW);
    digitalWrite(L_LENPin, LOW);
    digitalWrite(R_RENPin, LOW);
    digitalWrite(R_LENPin, LOW);
}

void Motor::rotate(int pwm, float degree)
{
    validate_pwm(pwm);

    float dgrspeed = calculate_dgrspeed(pwm);
    float abs_dgr = degree;
    if(abs_dgr <0){
        abs_dgr *= -1.0f;
    }
    float delaytime = abs_dgr / dgrspeed;
    if (degree > 0) 
    {
        rmotor_run(pwm, false); // 오른쪽 바퀴는 뒤로 회전
        lmotor_run(pwm); // 왼쪽 바퀴는 앞으로 회전
    }
    else {
        rmotor_run(pwm); // 오른쪽 바퀴는 앞으로 회전
        lmotor_run(pwm, false); // 왼쪽 바퀴는 뒤로 회전
    }
    // 모터 최대 150rpm, 설정 최대 pwm 100, 6.0f는 초당 각속도 계산을 위한 상수
    delay(delaytime);
}

// 커브 함수 동작 지침
// 1. degree값만큼 회전
// 2. 각 바퀴가 pwm값만큼 회전 (딜레이 시간 계산 필요)
// 3. -degree값만큼 회전

// l_pwm, r_pwm 말고, 원하는 pwm값 (오,왼 평균pwm), 각도를 설정해서 커브를 구현하는건 어떨까?

void curve_avoid(float distance, int pwm, float degree, bool recover = false)
{
    float currentdgr = 0.01f;
    int current_rpwm = pwm;
    int current_lpwm = pwm;
    std::vector<std::pair<int, int>> pwmList;
    if (distance > avoidDistance / 2.0f) {
        float addWeight = 0.02f;
        while (currentdgr < degree) {
            currentdgr += addWeight;
            addWeight += 0.3f;

            // 각도 범위 변경 시 이 부분도 유의
            if (degree > 0) {
                calculate_twin_pwm(pwm, currentdgr, &current_lpwm, &current_rpwm);
            }
            else {
                calculate_twin_pwm(pwm, currentdgr, &current_rpwm, &current_lpwm);
            }
            std::cout << "currentdgr : " << currentdgr << ", current_lpwm : " << current_lpwm << ", current_rpwm : " << current_rpwm << "\n";
            pwmList.push_back({ current_lpwm, current_rpwm });
        }
        std::cout << "reached target degree : " << currentdgr << "\n";
        int index = pwmList.size() - 1;

        //아래는 회피 후 복귀용 코드..
        while (index >= 0) {
            std::cout << " current_lpwm : " << pwmList[index].first << ", current_rpwm : " << pwmList[index].second << "\n";
            index--;
        }
    }
    else if (distance < avoidDistance / 1.3f) {

    }
}

void Motor::curve_coner(int pwm, float degree)
{
}

#pragma endregion

int main() {
    Motor motor;
    int i=3;
    while(i>0){
        i--;
        motor.straight(70);
        delay(300);
        motor.backoff(50);
        delay(200);
        motor.rotate(50, 30);
    }

    motor.stop();
    
    return 0;
}