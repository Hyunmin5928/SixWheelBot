#include "motorCtrl.h"
#include <wiringPi.h>
#include <softPwm.h>
#include <iostream>
#include <cmath>
#include <vector>

#define PI 3.1415926 

#define L_RPWM 23//1
#define L_LPWM 24//23
#define R_RPWM 1//24
#define R_LPWM 2//26

#define L_REN 21
#define L_LEN 22
#define R_REN 6
#define R_LEN 10

#define maxPulse 1024
#define maxSpeed 150.0f

#define avoidDistance_trigger 300.0f //cm
#define avoidDistance_step1 250.0f
#define avoidDistance_step2 180.0f
#define wheelInterval 31.0f //cm

//degree 값을 180 ~ -180 으로 설정함. 0~360으로 나올 경우 변수 변경 필요 (실제 받아오는 값은 전방 부채꼴 각도이므로, 유의)

//@brief pwm값 유효성 확인
#define validate_pwm(pwm)                         \
    if (!pwm_isvalid(pwm)) {                            \
        std::cout << "pwm value is invaild\n";       \
        return;                                                      \
    }

// private
#pragma region Private functions

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
    return atan(degree * PI /180);
}

void Motor::calculate_twin_pwm(float coredistance, int pwm, float degree, int* pwm1, int* pwm2)
{
    float pwm1_r = coredistance + wheelInterval/2.0f;
    float pwm2_r = coredistance - wheelInterval/2.0f;
    if(degree ==0){
        *pwm1 = pwm;
        *pwm2 = pwm;
        return;
    }
    *pwm1 = pwm * pwm1_r/coredistance;
    *pwm2 = pwm * pwm2_r/coredistance;
    if(pwm_isvalid(*pwm1)==false){
        std::cout<<"watch out : pwm1 is over maxPwm value\n";
    }
    
}

void Motor::lmotor_run(int pwm, bool front=true)
{
    std::cout<<"Lmotor_run\n";
    validate_pwm(pwm);
    if (front) {

        pwmWrite(L_RpwmPin, pwm); //positive forward
        pwmWrite(L_LpwmPin, 0); // must turn off this pin when using L_pwmPin
    }
    else {
        pwmWrite(L_RpwmPin, 0); //negative backward
        pwmWrite(L_LpwmPin, pwm);
    }

}

void Motor::rmotor_run(int pwm, bool front = true)
{
    std::cout << "Rmotor_run\n";
    validate_pwm(pwm);
    if (front) {
        pwmWrite(R_RpwmPin, pwm); //positive forward
        pwmWrite(R_LpwmPin, 0); // must turn off this pin when using L_pwmPin
    }
    else {
        pwmWrite(R_RpwmPin, 0); //negative backward
        pwmWrite(R_LpwmPin, pwm);
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

    pinMode(L_RpwmPin, PWM_OUTPUT);
    pinMode(L_LpwmPin, PWM_OUTPUT);
    pinMode(R_RpwmPin, PWM_OUTPUT);
    pinMode(R_LpwmPin, PWM_OUTPUT);
    pinMode(R_RENPin, OUTPUT);
    pinMode(R_LENPin, OUTPUT);
    pinMode(L_RENPin, OUTPUT);
    pinMode(L_LENPin, OUTPUT);

    pwmSetMode(PWM_MODE_MS);
    //pwm max pulse
    pwmSetRange(1024);
    pwmSetClock(32);
    
    digitalWrite(L_RENPin, HIGH);
    digitalWrite(L_LENPin, HIGH);
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
    pwmWrite(R_RpwmPin, pwm);   //positive forward
    pwmWrite(R_LpwmPin, 0);     // must turn off this pin when using R_pwmPin
    pwmWrite(L_RpwmPin, pwm);   //positive forward
    pwmWrite(L_LpwmPin, 0);     //must turn off this pin when using R_pwmPin
}

void Motor::backoff(int pwm)
{
    validate_pwm(pwm);
    pwmWrite(R_LpwmPin, pwm);
    pwmWrite(R_RpwmPin, 0);
    pwmWrite(L_LpwmPin, pwm);
    pwmWrite(L_RpwmPin, 0);
}

void Motor::stop(){
    pwmWrite(R_RpwmPin,0);
    pwmWrite(R_LpwmPin, 0);
    pwmWrite(L_RpwmPin,0);
    pwmWrite(L_LpwmPin, 0);
    digitalWrite(R_LENPin, LOW);
    digitalWrite(L_LENPin, LOW);
    digitalWrite(R_RENPin, LOW);
    digitalWrite(R_LENPin, LOW);
}

void Motor::rotate(int pwm, float degree)
{
    validate_pwm(pwm);

    float dgrspeed = calculate_dgrspeed(pwm);
    //abs_dgr : 절대값 각도
    float abs_dgr = degree;
    if(abs_dgr <0){
        abs_dgr *= -1.0f;
    }
    float delaytime = abs_dgr / dgrspeed;

    unsigned int currentTime = millis();
    while(millis()-currentTime < delaytime){
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
    }
}

//동작 중에 추가로 피해야할 대상이 나타나면 유연하게 회피하도록 해야함
void Motor::curve_avoid(float distance, int pwm, float degree, bool recover = false)
{
    float currentDegree=0;
    if(distance<=avoidDistance_trigger){
        //if(degree>)
        //curve_corner(distance, pwm, 20);
        //if(distance<=avoidDistance_step1){}
    }

    /*
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
                calculate_twin_pwm(distance, pwm, currentdgr, &current_lpwm, &current_rpwm);
            }
            else {
                calculate_twin_pwm(distance, pwm, currentdgr, &current_rpwm, &current_lpwm);
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
    */
}

void Motor::curve_corner(float connerdistance, int pwm, float degree)
{
    validate_pwm(pwm);
    float dgrspeed = calculate_dgrspeed(pwm);
    float abs_dgr = degree;
    if(degree < 0){
        abs_dgr *= -1.0f;
    }
    float delaytime = abs_dgr / dgrspeed;

    int pwm_1, pwm_2;
    calculate_twin_pwm(connerdistance, pwm, degree, &pwm_1, &pwm_2);

    if(degree>0){
        pwmWrite(L_RpwmPin, pwm_1);
        pwmWrite(R_RpwmPin, pwm_2);
    }else{
        pwmWrite(L_RpwmPin, pwm_2);
        pwmWrite(R_RpwmPin, pwm_1);
    }
    
}

#pragma endregion

int main() {
    Motor motor;
    
    unsigned int time = millis();
    while(millis()-time <3000){
        
        motor.straight(1024);
    }
    time=millis();
    while(millis()-time <3000){

        motor.backoff(1024);
    }
    time=millis();
    while(millis()-time <3000){
        motor.curve_corner(100.0f, 500, 90);
    }

    motor.stop();
    
    return 0;
}