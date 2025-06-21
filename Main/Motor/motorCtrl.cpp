#include "motorCtrl.h"
#define DISABLE_WIRINGPI_DELAY
#include <wiringPi.h>
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

#define avoidDistance_trigger 800.0f //cm
#define avoidDistance_step1 1000.0f
#define avoidDistance_step2 800.0f
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

        softPwmWrite(L_RpwmPin, pwm); //positive forward
        softPwmWrite(L_LpwmPin, 0); // must turn off this pin when using L_pwmPin
    }
    else {
        softPwmWrite(L_RpwmPin, 0); //negative backward
        softPwmWrite(L_LpwmPin, pwm);
    }

}

void Motor::rmotor_run(int pwm, bool front = true)
{
    std::cout << "Rmotor_run\n";
    validate_pwm(pwm);
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

    softPwmCreate(L_RpwmPin, 0, maxPulse);
    softPwmCreate(L_LpwmPin, 0, maxPulse);
    softPwmCreate(R_RpwmPin, 0, maxPulse);
    softPwmCreate(R_LpwmPin, 0, maxPulse);
    
    digitalWrite(L_RENPin, HIGH);
    digitalWrite(L_LENPin, HIGH);
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
}

Motor::Motor(int lr_pwmPin, int ll_pwmPin,int rr_pwmPin, int rl_pwmPin, int rr_enPin, int rl_enPin, int lr_enPin, int ll_enPin){
    motor_setup(lr_pwmPin, ll_pwmPin, rr_pwmPin, rl_pwmPin, rr_enPin, rl_enPin, lr_enPin, ll_enPin);
    std::cout << "motor set up : user setting\n";
}

Motor::~Motor(){
}
#pragma endregion

#pragma region Move functions



void Motor::straight(int pwm)
{
    Logger::instance().debug("motor", "[MotorCtrl] Straight");
    validate_pwm(pwm);
    softPwmWrite(R_RpwmPin, pwm);   //positive forward
    softPwmWrite(R_LpwmPin, 0);     // must turn off this pin when using R_pwmPin
    softPwmWrite(L_RpwmPin, pwm);   //positive forward
    softPwmWrite(L_LpwmPin, 0);     //must turn off this pin when using R_pwmPin
}

void Motor::backoff(int pwm)
{
    Logger::instance().debug("motor", "[MotorCtrl] backoff");
    validate_pwm(pwm);
    softPwmWrite(R_LpwmPin, pwm);
    softPwmWrite(R_RpwmPin, 0);
    softPwmWrite(L_LpwmPin, pwm);
    softPwmWrite(L_RpwmPin, 0);
}

void Motor::stop(){
    Logger::instance().debug("motor", "[MotorCtrl] stop");
    softPwmWrite(R_RpwmPin,0);
    softPwmWrite(R_LpwmPin, 0);
    softPwmWrite(L_RpwmPin,0);
    softPwmWrite(L_LpwmPin, 0);
    digitalWrite(R_LENPin, LOW);
    digitalWrite(L_LENPin, LOW);
    digitalWrite(R_RENPin, LOW);
    digitalWrite(R_LENPin, LOW);
}

void Motor::rotate(int pwm, float degree)
{
    Logger::instance().debug("motor", "[MotorCtrl] rotate");
    validate_pwm(pwm);

    digitalWrite(L_RENPin, HIGH);
    digitalWrite(L_LENPin, HIGH);
    digitalWrite(R_RENPin, HIGH);
    digitalWrite(R_LENPin, HIGH);

    float curDgr;
    float targetDgr = fmod((curDgr+degree), 360.0f);
    if(targetDgr < 0.0f) targetDgr += 360.0f;

    if(degree < 0.0f){
        while(curDgr>targetDgr){
            //curDgr 업데이트 코드를 넣어주세요
            lmotor_run(pwm,false);
            rmotor_run(pwm);
        }
    }
    else{
        while(curDgr<targetDgr){
            lmotor_run(pwm);
            rmotor_run(pwm, false);
        }
    }
}

//동작 중에 추가로 피해야할 대상이 나타나면 유연하게 회피하도록 해야함
void Motor::curve_avoid(float distance, int pwm, float degree, bool recover = false)
{
    float abs_degree = degree>0 ? degree : -degree;
    float avoid_degree = 30.0f-abs_degree;
    if(distance<=avoidDistance_trigger && degree >= -20.0f && degree <= 20.0f){
        
        if(degree > 0){ //왼쪽 회피
            rotate(pwm, avoid_degree);
            Logger::instance().debug("motor", "[MotorCtrl] rotate left for avoid : avoid angle");
        }
        else{ //오른쪽 회피
            rotate(pwm, -avoid_degree);
            Logger::instance().debug("motor", "[MotorCtrl] rotate right for avoid : avoid angle");
        }
    }
    long long unsigned int currentTime = millis();
    straight(pwm);
    Logger::instance().debug("motor", "[MotorCtrl] straight for avoid");
    while(millis() - currentTime < 500) {

    }
    stop();
    if(degree > 0){ //왼쪽 회피
        rotate(pwm, -avoid_degree);
        Logger::instance().debug("motor", "[MotorCtrl] rotate left for avoid : recover angle");
    }
    else {
        rotate(pwm, avoid_degree);
        Logger::instance().debug("motor", "[MotorCtrl] rotate right for avoid : recover angle");
    }

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

    unsigned int currentTime = millis();
    if(degree>0){
        while(millis()-currentTime <= delaytime){
            softPwmWrite(L_RpwmPin, pwm_1);
            softPwmWrite(R_RpwmPin, pwm_2);
        }
        
    }else{
        while(millis()-currentTime <= delaytime){
            softPwmWrite(L_RpwmPin, pwm_2);
            softPwmWrite(R_RpwmPin, pwm_1);
        }
    }
    
}

#pragma endregion

int Operation() {
    Motor motor;
    Lidar lidar;
    lidar.scan_oneCycle();

    if(lidar.get_nearPoint().angle<20.0f && lidar.get_nearPoint().angle > -20.0f && lidar.get_nearPoint().range < avoidDistance_trigger){
        motor.curve_avoid(lidar.get_nearPoint().range, 700, lidar.get_nearPoint().angle);
    }
    delay(2000);
    /*
    for(int i=0; i<scanpoints.size(); i++){
        
        if(scanpoints[i].angle <20.0f && scanpoints[i].angle > -20.0f && scanpoints[i].range < avoidDistance_trigger){
            if(scanpoints[i].range < avoid_dist && scanpoints[i].range != 0.0f){
                avoid_dgr=scanpoints[i].angle;
                avoid_dist = scanpoints[i].range;
            }
        }
    }
    log_msg("Debug", "scanpoint near : "+std::to_string(avoid_dgr)+", "+std::to_string(avoid_dist));
    */

    

    //motor.curve_avoid(avoid_dist, 700, avoid_dgr);

    //scanpoint 자동 업데이트
    //너무 빠른 업데이트를 막기위해 delay넣어야하는지는 실제 테스트해봐야함
    /*
    unsigned int time = millis();
    while(millis()-time <5000){
        motor.scan_OneCycle();
        motor.straight(1024);
    }
    time=millis();
    while(millis()-time <3000){

        motor.backoff(1024);
    }
    time=millis();
    motor.get_scanpoints()
    while(millis()-time <3000){
        motor.curve_corner(100.0f, 500, 90);
    }
    */
    motor.stop();
    return 0;
}
