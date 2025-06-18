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

void Motor::log_msg(const std::string& level, const std::string& msg) {
    auto now = std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now());
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&now),
                                "%Y-%m-%d %H:%M:%S")
                << " [" << level << "] " << msg << "\n";
    write(log_fd, oss.str().c_str(), oss.str().size());
}

void Motor::open_log_file(){
    log_fd = open(LOG_FILE.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd < 0) {
        std::cerr << "Failed to open log file: " << LOG_FILE << std::endl;
    } else {
        log_msg("INFO", "Log file opened successfully.");
    }
}

void Motor::close_log_file() {
    if (log_fd >= 0) {
        close(log_fd);
        log_fd = -1;
        log_msg("INFO", "Log file closed successfully.");
        } 
        else {
            std::cerr << "Log file is not open." << std::endl;
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
    open_log_file();
    log_msg("INFO", "Motor setup");
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
    log_msg("INFO", "Motor destructor called, cleaning up resources.");
    close_log_file();
    stop();
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
    float abs_degree = degree>0 ? degree : -degree;
    float avoid_degree = 30.0f-abs_degree;
    if(distance<=avoidDistance_trigger && degree >= -20.0f && degree <= 20.0f){
        
        if(degree > 0){ //왼쪽 회피
            rotate(pwm, avoid_degree);
            log_msg("Debug", "rotate left for avoid : avoid angle");
            std::cout << "rotate left for avoid : avoid angle\n";
        }
        else{ //오른쪽 회피
            rotate(pwm, -avoid_degree);
            log_msg("Debug", "rotate right for avoid : avoid angle");
            std::cout << "rotate right for avoid : avoid angle\n";
        }
    }
    long long unsigned int currentTime = millis();
    straight(pwm);
    log_msg("Debug", "straight for avoid");
    std::cout << "straight for avoid\n";
    while(millis() - currentTime < 500) {

    }
    stop();
    if(degree > 0){ //왼쪽 회피
        rotate(pwm, -avoid_degree);
        log_msg("Debug", "rotate left for avoid : recover angle");
        std::cout << "rotate left for avoid : recover angle\n";
    }
    else {
        rotate(pwm, avoid_degree);
        log_msg("Debug", "rotate right for avoid : recover angle");
        std::cout << "rotate right for avoid : recover angle\n";
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

    unsigned int currentTime = millis();
    if(degree>0){
        while(millis()-currentTime <= delaytime){
            pwmWrite(L_RpwmPin, pwm_1);
            pwmWrite(R_RpwmPin, pwm_2);
        }
        
    }else{
        while(millis()-currentTime <= delaytime){
            pwmWrite(L_RpwmPin, pwm_2);
            pwmWrite(R_RpwmPin, pwm_1);
        }
    }
    
}

#pragma endregion

int main() {
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