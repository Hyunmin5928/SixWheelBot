#ifndef MOTORCTRL_H
#define MOTORCTRL_H
#define DISABLE_WIRINGPI_DELAY
#include <wiringPi.h>
#include "motorCtrl.h"
#ifdef __MOTORCTRL_H__
#define PI 3.1415926 

#define L_RPWM 23//GPIO 13/
#define L_LPWM 24//GPIO 19/
#define R_RPWM 1//GPIO 18
#define R_LPWM 26//GPIO 12

#define L_REN 21 //GPIO 5/
#define L_LEN 22 //GPIO 6/
#define R_REN 6 //GPIO 25/
#define R_LEN 10 //GPIO 8/

#define maxPulse 100
#define maxSpeed 150.0f

#define avoidDistance_trigger 300.0f //cm
#define avoidDistance_step1 250.0f
#define avoidDistance_step2 180.0f
#define wheelInterval 31.0f //cm
#define ROTATION_CORRECTION 1.2f //회전 시 작동 시간 보정값

#endif


#endif

//degree 값을 180 ~ -180 으로 설정함. 0~360으로 나올 경우 변수 변경 필요 (실제 받아오는 값은 전방 부채꼴 각도이므로, 유의)

//@brief pwm값 유효성 확인
#define validate_pwm(pwm)                         \
    if (!pwm_isvalid(pwm)) {                            \
        std::cout << "pwm value is invaild\n";       \
        return;                                                      \
    }

// private
#pragma region Private functions

<<<<<<< HEAD
=======
LaserScan::points Motor::set_scanpoints(LaserScan& scan){
    scanData=scan.points;
    return scan.points;
}

int Motor::lidar_setup(){
    std::string port;
    ydlidar::os_init();

    std::map<std::string, std::string> ports =
        ydlidar::lidarPortList();
    std::map<std::string, std::string>::iterator it;

    if (ports.size() == 1)
    {
        port = ports.begin()->second;
    }
    else
    {
        int id = 0;

        for (it = ports.begin(); it != ports.end(); it++)
        {
        printf("[%d] %s %s\n", id, it->first.c_str(), it->second.c_str());
        id++;
        }

        if (ports.empty()){
        printf("Not Lidar was detected. Please enter the lidar serial port:");
        std::cin >> port;
        }
        else {
            while (ydlidar::os_isOk())
            {
                printf("Please select the lidar port:");
                std::string number;
                std::cin >> number;

                if ((size_t)atoi(number.c_str()) >= ports.size())
                {
                continue;
                }

                it = ports.begin();
                id = atoi(number.c_str());

                while (id)
                {
                id--;
                it++;
                }

                port = it->second;
                break;
            }
        }
    }

    int baudrate = 128000;
    bool isSingleChannel = false;
    float frequency = 6.0;

    if(!ydlidar::os_isOk())
        return 0;

    lidar.setlidaropt(LidarPropSerialPort, port.c_str(), port.size());
    /// ignore array
    std::string ignore_array;
    ignore_array.clear();
    lidar.setlidaropt(LidarPropIgnoreArray, ignore_array.c_str(),
                        ignore_array.size());

    //////////////////////int property/////////////////
    /// lidar baudrate
    lidar.setlidaropt(LidarPropSerialBaudrate, &baudrate, sizeof(int));
    int optval = TYPE_TRIANGLE;
    lidar.setlidaropt(LidarPropLidarType, &optval, sizeof(int));
    optval = YDLIDAR_TYPE_SERIAL;
    lidar.setlidaropt(LidarPropDeviceType, &optval, sizeof(int));
    /// sample rate
    optval = isSingleChannel ? 3 : 4;
    optval = 5;
    lidar.setlidaropt(LidarPropSampleRate, &optval, sizeof(int));
    /// abnormal count
    optval = 4;
    lidar.setlidaropt(LidarPropAbnormalCheckCount, &optval, sizeof(int));
    /// Intenstiy bit count
    optval = 10;
    lidar.setlidaropt(LidarPropIntenstiyBit, &optval, sizeof(int));

    //////////////////////bool property/////////////////
    /// fixed angle resolution
    bool b_optvalue = false;
    lidar.setlidaropt(LidarPropFixedResolution, &b_optvalue, sizeof(bool));
    /// rotate 180
    b_optvalue = false;
    lidar.setlidaropt(LidarPropReversion, &b_optvalue, sizeof(bool));
    /// Counterclockwise
    b_optvalue = false;
    lidar.setlidaropt(LidarPropInverted, &b_optvalue, sizeof(bool));
    b_optvalue = true;
    lidar.setlidaropt(LidarPropAutoReconnect, &b_optvalue, sizeof(bool));
    /// one-way communication
    b_optvalue = false;
    lidar.setlidaropt(LidarPropSingleChannel, &isSingleChannel, sizeof(bool));
    /// intensity
    b_optvalue = false;
    lidar.setlidaropt(LidarPropIntenstiy, &b_optvalue, sizeof(bool));
    /// Motor DTR
    b_optvalue = true;
    lidar.setlidaropt(LidarPropSupportMotorDtrCtrl, &b_optvalue, sizeof(bool));
    /// HeartBeat
    b_optvalue = false;
    lidar.setlidaropt(LidarPropSupportHeartBeat, &b_optvalue, sizeof(bool));

    //////////////////////float property/////////////////
    /// unit: °
    float f_optvalue = 180.0f;
    lidar.setlidaropt(LidarPropMaxAngle, &f_optvalue, sizeof(float));
    f_optvalue = -180.0f;
    lidar.setlidaropt(LidarPropMinAngle, &f_optvalue, sizeof(float));
    f_optvalue = 64.f;
    lidar.setlidaropt(LidarPropMaxRange, &f_optvalue, sizeof(float));
    f_optvalue = 0.05f;
    lidar.setlidaropt(LidarPropMinRange, &f_optvalue, sizeof(float));
    lidar.setlidaropt(LidarPropScanFrequency, &frequency, sizeof(float));
    lidar.enableGlassNoise(false);
    lidar.enableSunNoise(false);
    lidar.setBottomPriority(true);

    unit32_t t=getms();
    int c=0;
    bool ret=lidar.initialize();
    if (!ret)
    {
        fprintf(stderr, "Fail to initialize %s\n", lidar.DescribeError());
        fflush(stderr);
        return -1;
    }

    ret = lidar.turnOn();
    if (!ret)
    {
        fprintf(stderr, "Fail to start %s\n", lidar.DescribeError());
        fflush(stderr);
        return -1;
    }

    if (ret)
    {
        device_info di;
        memset(&di, 0, DEVICEINFOSIZE);
        if (lidar.getDeviceInfo(di, EPT_Module)) {
        ydlidar::core::common::printfDeviceInfo(di, EPT_Module);
        }
        else {
        printf("Fail to get module device info\n");
        }

        if (lidar.getDeviceInfo(di, EPT_Base)) {
        ydlidar::core::common::printfDeviceInfo(di, EPT_Base);
        }
        else {
        printf("Fail to get baseplate device info\n");
        }
    }
    return 0;
}

>>>>>>> 6872a42 (CYdLidar 클래스 넣어서 라이다 초기화 + 설정 + 1회만 가동할 수 있도록 함. 모터 클래스에서 스캔 값 가지고 올 수 있으나, 임계값 이하의 거리 데이터만 따로 필터링해서 모터 동작하도록 해야함.)
float Motor::calculate_dgrspeed(int pwm)
{
    std::cout<<"maxSpeed : "<<maxSpeed<<"\n";
    std::cout<<"maxPulse : "<<maxPulse<<"\n";
    return 900.0f *(static_cast<float>(pwm)/maxPulse); 

    //초당 150rpm 최대 이동 >> 82 cm/sec
}

float Motor::average_pwm(int l_pwm, int r_pwm)
{
    return (l_pwm + r_pwm) / 2.0f;
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

void Motor::lmotor_run(int pwm, bool front)
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

void Motor::rmotor_run(int pwm, bool front)
{
    // std::cout << "Rmotor_run\n";
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

void Motor::calculate_ang_dist()
{
  for(int i = 0; i < scanData.points.size(); i++){
    scanData.points[i].angle = scanData.points[i].angle *180.0 / M_PI; //각도값을 radian으로 변환
    scanData.points[i].range = scanData.points[i].range * 1000.0; //거리값을 mm로 변환
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
    
<<<<<<< HEAD
    softPwmCreate(L_RpwmPin, 0, maxPulse);
    softPwmCreate(L_LpwmPin, 0, maxPulse);
    softPwmCreate(R_RpwmPin, 0, maxPulse);
    softPwmCreate(R_LpwmPin, 0, maxPulse);
=======
    digitalWrite(L_RENPin, HIGH);
    digitalWrite(L_LENPin, HIGH);

    lidar_setup();
>>>>>>> 6872a42 (CYdLidar 클래스 넣어서 라이다 초기화 + 설정 + 1회만 가동할 수 있도록 함. 모터 클래스에서 스캔 값 가지고 올 수 있으나, 임계값 이하의 거리 데이터만 따로 필터링해서 모터 동작하도록 해야함.)
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

<<<<<<< HEAD
// std::vector<LaserPoint> Motor::set_scanpoints(LaserScan& scan){
//     scanData=scan.points;
//     for(int i=0; i<scanData.size(); i++){
//         std::cout<<"scanData : "<<scanData[i].angle<<", "<<scanData[i].range<<"\n";
//     }
//     std::cout<<"--------------end scan data----------------\n";
//     return scan.points;
// }
=======
void Motor::scan_oneCycle(){
    lidar.turnOn();
    if(ydlidar::os_isOk()){
        if(lidar.doProcessSimple(scanData)){
            usableData.clear();
            for(size_t i=0; i<scanData.points.size(); i++){
                const lidarPoint &p = scanData.points.at(i);
                LaserPoint temp={p.angle*180.0/M_PI, p.range*1000.0, p.intensity};
                usableData.push_back(temp);
            }
        }
        else{
            fprintf(stderr, "Failed to get Lidar Data\n");
            fflush(stderr);
        }
        if(!c++){
            printf("Time consuming [%u] from initialization to parsing to point cloud data\n", getms() - t);
        }
    }
    lidar.turnOff();
}

std::vector<LaserPoint> Motor::get_scanData(){
    return usableData;
}

void Motor::show_scanData(){
    for(int i=0; i<usableData.size(); i++){
        std::cout<<usableData[i].angle<<usableData[i].range<<"\n";
    }
    std::cout<<"----* end of scan data *----\n";
}
>>>>>>> 6872a42 (CYdLidar 클래스 넣어서 라이다 초기화 + 설정 + 1회만 가동할 수 있도록 함. 모터 클래스에서 스캔 값 가지고 올 수 있으나, 임계값 이하의 거리 데이터만 따로 필터링해서 모터 동작하도록 해야함.)

void Motor::straight(int pwm)
{
    validate_pwm(pwm);
    digitalWrite(R_LENPin, HIGH);
    digitalWrite(L_LENPin, HIGH);
    digitalWrite(R_RENPin, HIGH);
    digitalWrite(L_RENPin, HIGH);
    std::cout << "rpwm go\n";
    softPwmWrite(R_RpwmPin, 0);   //positive forward
    softPwmWrite(R_LpwmPin, pwm);     // must turn off this pin when using R_pwmPin
    std::cout << "lpwm go\n";
    softPwmWrite(L_RpwmPin, pwm);   //positive forward
    softPwmWrite(L_LpwmPin, 0);     //must turn off this pin when using R_pwmPin
}

void Motor::backoff(int pwm)
{
    validate_pwm(pwm);
    digitalWrite(R_LENPin, HIGH);
    digitalWrite(R_RENPin, HIGH);
    digitalWrite(L_LENPin, HIGH);
    digitalWrite(L_RENPin, HIGH);
    std::cout << "rpwm back\n";
    softPwmWrite(R_RpwmPin, pwm);
    softPwmWrite(R_LpwmPin, 0);
    std::cout << "lpwm back\n";
    softPwmWrite(L_RpwmPin, 0);
    softPwmWrite(L_LpwmPin, pwm);
}

void Motor::stop(){
    std::cout << "rpwm stop\n";
    softPwmWrite(R_RpwmPin,0);
    softPwmWrite(R_LpwmPin, 0);
    std::cout << "lpwm stop\n";
    softPwmWrite(L_RpwmPin,0);
    softPwmWrite(L_LpwmPin, 0);
    digitalWrite(L_RENPin, LOW);
    digitalWrite(L_LENPin, LOW);
    digitalWrite(R_RENPin, LOW);
    digitalWrite(R_LENPin, LOW);
}

void Motor::rotate(int pwm, float degree)
{
    validate_pwm(pwm);
    digitalWrite(R_LENPin, HIGH);
    digitalWrite(L_LENPin, HIGH);
    digitalWrite(R_RENPin, HIGH);
    digitalWrite(L_RENPin, HIGH);
    
    float dgrspeed = calculate_dgrspeed(pwm);
    //abs_dgr : 절대값 각도
    float abs_dgr = (degree<0)? -degree : degree;
    if(degree == 0){
        std::cout << "degree is zero, no rotation\n";
        return;
    }
    float delaytime = abs_dgr / dgrspeed * ROTATION_CORRECTION;

    unsigned int currentTime = millis();
    std::cout<<"delaytime : "<<delaytime<<"\n";
    /*
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
    */
    stop();
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

    unsigned int currentTime = millis();
    if(degree>0){
        while(millis()-currentTime <= delaytime){
            softPwmWrite(L_RpwmPin, pwm_1);
            softPwmWrite(R_RpwmPin, pwm_2);
            softPwmWrite(L_LpwmPin, 0);
            softPwmWrite(R_LpwmPin, 0);
        }
        stop();
    }else{
        while(millis()-currentTime <= delaytime){
            softPwmWrite(L_RpwmPin, pwm_2);
            softPwmWrite(R_RpwmPin, pwm_1);
            softPwmWrite(L_LpwmPin, 0);
            softPwmWrite(R_LpwmPin, 0);
        }
        stop();
    }
    
}

#pragma endregion

int main() {
    Motor motor;
<<<<<<< HEAD
    // Lidar lidar(&motor);
    // lidar.start();
    // lidar.lidar_only_Cycle();
    // delay(1000);
    // std::cout<<"Lidar cycle done\n";
    std::string port;
    ydlidar::os_init();

    std::map<std::string, std::string> ports =
      ydlidar::lidarPortList();
    std::map<std::string, std::string>::iterator it;

  if (ports.size() == 1)
  {
    port = ports.begin()->second;
  }
  else
  {
    int id = 0;

    for (it = ports.begin(); it != ports.end(); it++)
    {
      printf("[%d] %s %s\n", id, it->first.c_str(), it->second.c_str());
      id++;
    }

    if (ports.empty())
    {
      printf("Not Lidar was detected. Please enter the lidar serial port:");
      std::cin >> port;
    }
    else
    {
      while (ydlidar::os_isOk())
      {
        printf("Please select the lidar port:");
        std::string number;
        std::cin >> number;

        if ((size_t)atoi(number.c_str()) >= ports.size())
        {
          continue;
        }

        it = ports.begin();
        id = atoi(number.c_str());

        while (id)
        {
          id--;
          it++;
        }

        port = it->second;
        break;
      }
    }
  }

  int baudrate = 128000;

  while (ydlidar::os_isOk())
  {
    break;
  }

  if (!ydlidar::os_isOk())
    return 0;

  //单通还是双通
  bool isSingleChannel = false;
  // std::string input_channel;
  // printf("Whether the Lidar is one-way communication [yes/no]:");
  // std::cin >> input_channel;
  // std::transform(input_channel.begin(), input_channel.end(),
  //                input_channel.begin(),
  //                [](unsigned char c)
  //                {
  //                  return std::tolower(c); // correct
  //                });
  // if (input_channel.find("y") != std::string::npos)
  //   isSingleChannel = true;

  if (!ydlidar::os_isOk())
    return 0;

  //转速
  float frequency = 5.0;
  std::string input_frequency ="6";
  while (ydlidar::os_isOk() && !isSingleChannel)
  {
    
    frequency = atof(input_frequency.c_str());
    if (frequency <= 12 && frequency >= 5.0)
    {
      break;
    }
    fprintf(stderr, "Invalid scan frequency,"
      "The scanning frequency range is 5 to 12 HZ, Please re-enter.\n");
  }

  if (!ydlidar::os_isOk())
    return 0;

  CYdLidar laser;
  //////////////////////string property/////////////////
  /// lidar port
  laser.setlidaropt(LidarPropSerialPort, port.c_str(), port.size());
  /// ignore array
  std::string ignore_array;
  ignore_array.clear();
  laser.setlidaropt(LidarPropIgnoreArray, ignore_array.c_str(),
                    ignore_array.size());

  //////////////////////int property/////////////////
  /// lidar baudrate
  laser.setlidaropt(LidarPropSerialBaudrate, &baudrate, sizeof(int));
  /// tof lidar
  int optval = TYPE_TRIANGLE;
  laser.setlidaropt(LidarPropLidarType, &optval, sizeof(int));
  /// device type
  optval = YDLIDAR_TYPE_SERIAL;
  laser.setlidaropt(LidarPropDeviceType, &optval, sizeof(int));
  /// sample rate
  optval = isSingleChannel ? 3 : 4;
  optval = 5;
  laser.setlidaropt(LidarPropSampleRate, &optval, sizeof(int));
  /// abnormal count
  optval = 4;
  laser.setlidaropt(LidarPropAbnormalCheckCount, &optval, sizeof(int));
  /// Intenstiy bit count
  optval = 10;
  laser.setlidaropt(LidarPropIntenstiyBit, &optval, sizeof(int));

  //////////////////////bool property/////////////////
  /// fixed angle resolution
  bool b_optvalue = false;
  laser.setlidaropt(LidarPropFixedResolution, &b_optvalue, sizeof(bool));
  /// rotate 180
  b_optvalue = false;
  laser.setlidaropt(LidarPropReversion, &b_optvalue, sizeof(bool));
  /// Counterclockwise
  b_optvalue = false;
  laser.setlidaropt(LidarPropInverted, &b_optvalue, sizeof(bool));
  b_optvalue = true;
  laser.setlidaropt(LidarPropAutoReconnect, &b_optvalue, sizeof(bool));
  /// one-way communication
  b_optvalue = false;
  laser.setlidaropt(LidarPropSingleChannel, &isSingleChannel, sizeof(bool));
  /// intensity
  b_optvalue = false;
  laser.setlidaropt(LidarPropIntenstiy, &b_optvalue, sizeof(bool));
  /// Motor DTR
  b_optvalue = true;
  laser.setlidaropt(LidarPropSupportMotorDtrCtrl, &b_optvalue, sizeof(bool));
  /// HeartBeat
  b_optvalue = false;
  laser.setlidaropt(LidarPropSupportHeartBeat, &b_optvalue, sizeof(bool));

  //////////////////////float property/////////////////
  /// unit: °
  float f_optvalue = 180.0f;
  laser.setlidaropt(LidarPropMaxAngle, &f_optvalue, sizeof(float));
  f_optvalue = -180.0f;
  laser.setlidaropt(LidarPropMinAngle, &f_optvalue, sizeof(float));
  /// unit: m
  f_optvalue = 64.f;
  laser.setlidaropt(LidarPropMaxRange, &f_optvalue, sizeof(float));
  f_optvalue = 0.05f;
  laser.setlidaropt(LidarPropMinRange, &f_optvalue, sizeof(float));
  /// unit: Hz
  laser.setlidaropt(LidarPropScanFrequency, &frequency, sizeof(float));

  //禁用阳光玻璃过滤
  laser.enableGlassNoise(false);
  laser.enableSunNoise(false);

  //设置是否获取底板设备信息（默认仅尝试获取模组设备信息）
  laser.setBottomPriority(true);
  //启用调试
  // laser.setEnableDebug(true);

  uint32_t t = getms(); //时间
  int c = 0; //计数

  bool ret = laser.initialize();
  if (!ret)
  {
    fprintf(stderr, "Fail to initialize %s\n", laser.DescribeError());
    fflush(stderr);
    return -1;
  }

  ret = laser.turnOn();
  if (!ret)
  {
    fprintf(stderr, "Fail to start %s\n", laser.DescribeError());
    fflush(stderr);
    return -1;
  }


  if (ret)
  {
    device_info di;
    memset(&di, 0, DEVICEINFOSIZE);
    if (laser.getDeviceInfo(di, EPT_Module)) {
      ydlidar::core::common::printfDeviceInfo(di, EPT_Module);
    }
    else {
      printf("Fail to get module device info\n");
    }

    if (laser.getDeviceInfo(di, EPT_Base)) {
      ydlidar::core::common::printfDeviceInfo(di, EPT_Base);
    }
    else {
      printf("Fail to get baseplate device info\n");
    }
  }
  int cnt = 0;
  LaserScan scan;
  bool success= laser.doProcessSimple(scan);

  if(success){
    printf("Scan received [%llu] points scanFreq [%.02f]\n",
             scan.points.size(),
             scan.scanFreq);
      
      fflush(stdout);

      //왜 인식을 못함..?
      calculate_ang_dist();
  }
  else
    {
      fprintf(stderr, "Failed to get Lidar Data\n");
      fflush(stderr);
    }
    if (!c++)
    {
      printf("Time consuming [%u] from initialization to parsing to point cloud data\n",
        getms() - t);
    }

    /*
  while (ydlidar::os_isOk()&&cnt<2)
  {
    if (laser.doProcessSimple(scan))
    {
      printf("Scan received [%llu] points scanFreq [%.02f]\n",
             scan.points.size(),
             scan.scanFreq);
      for (size_t i = 0; i < scan.points.size(); ++i)
      {
        const LaserPoint &p = scan.points.at(i);
        scan.points[i].angle = p.angle * 180.0 / M_PI; //각도값을 degree로 변환
        scan.points[i].range = p.range * 1000.0; //거리값을 mm로 변환

      }
      fflush(stdout);
      cnt++;
    }
    
  }
    */

  //스캔값
  motor.scanData = scan;
  for(int i=0; i<scan.points.size(); i++){
      std::cout<<"scanData: "<<scan.points[i].angle<<", "<< scan.points[i].range<<"\n";
  }
  std::cout<<"--------------end scan data----------------\n";
  laser.turnOff();
  laser.disconnecting();
    //scanpoint 자동 업데이트
    //너무 빠른 업데이트를 막기위해 delay넣어야하는지는 실제 테스트해봐야함
    //직진 후진 회전 코너링 순서로 테스트
    unsigned int time = millis();
    //motor.rotate(50, 90); // 90도 회전
    //motor.straight(70); // 직진
    while(millis()-time <7000){
    }
=======
    motor.scan_oneCycle();
    //scanpoint 자동 업데이트
    //너무 빠른 업데이트를 막기위해 delay넣어야하는지는 실제 테스트해봐야함
    std::cout<<motor.show_scanData();

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
>>>>>>> 6872a42 (CYdLidar 클래스 넣어서 라이다 초기화 + 설정 + 1회만 가동할 수 있도록 함. 모터 클래스에서 스캔 값 가지고 올 수 있으나, 임계값 이하의 거리 데이터만 따로 필터링해서 모터 동작하도록 해야함.)
    motor.stop();
    // lidar.lidar_off();
    // time=millis();
    // motor.rotate(50, 90);
    // while(millis()-time<5000){
    //     // 5초 이후에도 멈추지 않으면
    // }
    // motor.stop();
    // //motor.get_scanpoints()

    // // motor.curve_corner(80.0f, 1000, 90);
    // // time=millis();
    // // while(millis()-time < 5000){
    // //     // 5초 이후에도 멈추지 않으면
    // // }
    // motor.stop();
    motor.stop();
    return 0;
}