#include <wiringPi.h>
#include <softPwm.h>
#include <iostream>
#include <motorCtrl.h>

#define RPWM 1
#define LPWM 23
#define LR_EN 4
#define LL_EN 5
#define RR_EN 29
#define RL_EN 31

#define maxPulse 255

// R, L MOtor 하나의 클래스에 넣어서 조져야함
// R, L Motor in one class

void Motor::motor_setup()
{
    wiringPiSetup();
    pinMode(RPWM, OUTPUT);
    pinMode(LPWM, OUTPUT);
    pinMode(LL_EN, OUTPUT);
    pinMode(LR_EN, OUTPUT);
    pinMode(RR_EN, OUTPUT);
    pinMode(RL_EN, OUTPUT);

    digitalWrite(RR_EN, HIGH);
    digitalWrite(RL_EN, LOW);
    digitalWrite(LL_EN,LOW);
    digitalWrite(LR_EN,HIGH);

    //R_Motor, L_Motor 0으로 초기화, 최대 펄스 255
    //R_Motor, L_Motor initialize 0, max purse 255
    softPwmCreate(RPWM, 0, maxPulse);
    softPwmCreate(LPWM, 0, maxPulse);
}

bool Motor::pwm_isvalid(int pwm)
{
    if(pwm > maxPulse || pwm < 0){
        return false;
    }else{
        return true;
    }
}

Motor::Motor()
{
}

Motor::Motor(int pwmPin, int R_EN, int L_EN){

}

Motor::~Motor(){

}

void Motor::straight(int pwm, int time)
{
    if(pwm_isvalid(pwm)==false){
        std::cout<<"pwm value is invaild\n";
        return;
    }
    else{
        softPwmWrite(RPWM, pwm);
        delay(time);
    }
}

void Motor::straight(int pwm, bool func)
{
}

void Motor::rotate(int pwm, int time)
{
}
