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

#define maxPulse 100
#define maxSpeed 150.0f

#define avoidDistance_trigger 800.0f //cm
#define avoidDistance_step1 1000.0f
#define avoidDistance_step2 800.0f
#define wheelInterval 310.0f //cm


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
    float rpm = (float)pwm / 100.0f * maxSpeed;  // PWM 비율로 rpm 계산
    float degPerSec = rpm * 360.0f / 60.0f;      // rpm -> deg/s 변환
    std::cout<<"\n"<<degPerSec<<"\n";

    return degPerSec;
}

float Motor::calculate_delaytime(int pwm, float degree){
    if(degree <0){
        degree = abs(degree);
    }
    float delayTimeMs = (degree / calculate_dgrspeed(pwm)) * 10000.0f; // 밀리초 단위
    std::cout<<"\n"<<delayTimeMs<<"\n";
    return delayTimeMs;
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
    validate_pwm(pwm);
    if (front) {
        softPwmWrite(R_RpwmPin, 0); //positive forward
        softPwmWrite(R_LpwmPin, pwm); // must turn off this pin when using L_pwmPin
    }
    else {
        softPwmWrite(R_RpwmPin, pwm); //negative backward
        softPwmWrite(R_LpwmPin, 0);
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
    
    softPwmCreate(L_RpwmPin,0,maxPulse);
    softPwmCreate(L_LpwmPin,0,maxPulse);
    softPwmCreate(R_LpwmPin,0,maxPulse);
    softPwmCreate(R_RpwmPin,0,maxPulse);

    digitalWrite(L_RENPin, HIGH);
    digitalWrite(L_LENPin, HIGH);
    digitalWrite(R_RENPin,HIGH);
    digitalWrite(R_LENPin, HIGH);

    std::cout << "Motor setup complete.\n";
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
}

Motor::Motor(int lr_pwmPin, int ll_pwmPin,int rr_pwmPin, int rl_pwmPin, int rr_enPin, int rl_enPin, int lr_enPin, int ll_enPin){
    motor_setup(lr_pwmPin, ll_pwmPin, rr_pwmPin, rl_pwmPin, rr_enPin, rl_enPin, lr_enPin, ll_enPin);
    std::cout << "motor set up : user setting\n";
}

Motor::~Motor(){
    stop();
}
#pragma endregion

#pragma region Move functions



void Motor::straight(int pwm)
{
    validate_pwm(pwm);
    digitalWrite(L_RENPin, HIGH);
    digitalWrite(L_LENPin, HIGH);
    digitalWrite(R_RENPin, HIGH);
    digitalWrite(R_LENPin, HIGH);
    softPwmWrite(L_RpwmPin, pwm);   //positive forward
    softPwmWrite(L_LpwmPin, 0);     //must turn off this pin when using R_pwmPin
    softPwmWrite(R_RpwmPin, 0);   //positive forward
    softPwmWrite(R_LpwmPin, pwm);     // must turn off this pin when using R_pwmPin
}

void Motor::backoff(int pwm)
{
    validate_pwm(pwm);
    digitalWrite(L_RENPin, HIGH);
    digitalWrite(L_LENPin, HIGH);
    digitalWrite(R_RENPin, HIGH);
    digitalWrite(R_LENPin, HIGH);
    softPwmWrite(R_LpwmPin, 0);
    softPwmWrite(R_RpwmPin, pwm);
    softPwmWrite(L_LpwmPin, pwm);
    softPwmWrite(L_RpwmPin, 0);
}

void Motor::stop(){
    softPwmWrite(R_RpwmPin,0);
    softPwmWrite(R_LpwmPin, 0);
    softPwmWrite(L_RpwmPin,0);
    softPwmWrite(L_LpwmPin, 0);
    digitalWrite(L_RENPin, LOW);
    digitalWrite(L_LENPin, LOW);
    digitalWrite(R_RENPin, LOW);
    digitalWrite(R_LENPin, LOW);
}

void Motor::rotate(int pwm, float degree)
{
    //degree는 돌고 싶은 각도 (상대각도)
    validate_pwm(pwm);

    digitalWrite(L_RENPin, HIGH);
    digitalWrite(L_LENPin, HIGH);
    digitalWrite(R_RENPin, HIGH);
    digitalWrite(R_LENPin, HIGH);

    /*
    //curDgr 받아와야해요
    float curDgr; 
    //curDgr = 회전하기 이전 yaw 값
    //target degree = yaw에서 얻은 값 + 원하는 값(회전 결과)
    float targetDgr = fmod((curDgr + degree), 360.0f);
    if (targetDgr < 0.0f) targetDgr += 360.0f;
    */
    unsigned long long time = millis();
    if(degree<0.0f){
        while(millis()-time < 3000){
            //curDgr 업데이트 코드 넣어주세요
            lmotor_run(pwm, false);
            rmotor_run(pwm, true);
        }
    }
    else{
        while(millis()-time < 3000){
            //curDgr 업데이트 코드 넣어주세요
            rmotor_run(pwm, false);
            lmotor_run(pwm, true);
        }
    }
    stop();
}

//동작 중에 추가로 피해야할 대상이 나타나면 유연하게 회피하도록 해야함
void Motor::curve_avoid(float distance, int pwm, float degree, bool recover = false)
{
    float abs_degree = degree>0 ? degree : -degree;
    float avoid_degree = 120.0f-abs_degree;
    if(distance<=avoidDistance_trigger && degree >= -60.0f && degree <= 60.0f){
        
        if(degree > 0){ //왼쪽 회피
            rotate(pwm, avoid_degree);
        }
        else{ //오른쪽 회피
            rotate(pwm, -avoid_degree);
        }
    }
    long long unsigned int currentTime = millis();
    straight(pwm);
    while(millis() - currentTime < 500) {

    }
    stop();
    if(degree > 0){ //왼쪽 회피
        rotate(pwm, -avoid_degree);
    }
    else {
        rotate(pwm, avoid_degree);
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
    //float delaytime =calculate_delaytime(pwm, degree);

    int pwm_1, pwm_2;
    calculate_twin_pwm(connerdistance, pwm, degree, &pwm_1, &pwm_2);

    unsigned int currentTime = millis();
    if(degree>0){
        while(millis()-currentTime <= 3000){
            softPwmWrite(L_RpwmPin, pwm_1);
            softPwmWrite(R_RpwmPin, pwm_2);
        }
        
    }else{
        while(millis()-currentTime <= 3000){
            softPwmWrite(L_RpwmPin, pwm_2);
            softPwmWrite(R_RpwmPin, pwm_1);
        }
    }
    
}

#pragma endregion

void run_Motor(){
    Motor motor;
}


int main() {
    Motor motor;
    long long unsigned int time = millis();
    //Lidar lidar;

    //lidar.scan_oneCycle();
    bool done = false;
    
    motor.straight(30);
    while(millis()-time < 1500){

    }
    motor.stop();
    motor.backoff(30);
    time = millis();
    while(millis()-time < 1500){

    }
    motor.stop();
    motor.rotate(40,90);
    motor.rotate(40,-90);
    
    motor.stop();
    
    
    //motor.stop();

    delay(2000);
    motor.stop();
    //motor.rotate(40, 90);
    //motor.stop();

    //motor.rotate(40, 90);

    /*
    while(1){
        motor.straight(40);
        while(millis()-time <100){
            lidar.scan_oneCycle();

            if(lidar.get_nearPoint().angle<60.0f && lidar.get_nearPoint().angle > -60.0f && lidar.get_nearPoint().range < avoidDistance_trigger && lidar.get_nearPoint().range>0.0f){
                std::cout << "Avoiding obstacle at angle: " << lidar.get_nearPoint().angle << ", distance: " << lidar.get_nearPoint().range << "\n";
                done = true;
            }
            if(done){
                std::cout<<"Rotate\n";
                delay(100);
                motor.stop();
                delay(100);
                //로테이트 함수가 안돌음
                break;
            }
         
        }   
        if(done){
            //여기 위치에서도 로테이트 함수가 안돌음
            
            std::cout<<"Straight\n";
            motor.straight(30);
            lidar.log_msg("Debug","Rotate done - Go Straight");
            break;
        }
        time = millis();
    }
    lidar.log_msg("Debug", "Rotate now");
    motor.rotate(40,90);
    long long unsigned curTime = millis();
    while(millis()-curTime<1500){
        motor.straight(40);
    }
    delay(100);
    motor.rotate(40,-90);
    curTime = millis();
    delay(100);
    while(millis()-curTime<1500){
        motor.straight(40);
    }

    lidar.log_msg("Debug", "Rotate done");

    motor.stop();
    return 0;
    */
}
