#pragma once
<<<<<<< HEAD
#include "../../YDLiDAR/YDLidar-SDK/src/CYdLidar.h"
// #include "../../YDLiDAR/YDLidar-SDK/examples/myung-ryun-lidar.h"
=======
#include "../LiDAR/YDLidar/YDLidar-SDK/src/CYdLidar.h"
//#include "../../YDLidar-SDK/examples/lidar_class.h"
#include <wiringPi.h>
>>>>>>> 6872a42 (CYdLidar 클래스 넣어서 라이다 초기화 + 설정 + 1회만 가동할 수 있도록 함. 모터 클래스에서 스캔 값 가지고 올 수 있으나, 임계값 이하의 거리 데이터만 따로 필터링해서 모터 동작하도록 해야함.)
#include <softPwm.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <functional>
#include <wiringPi.h>
#include <core/common/ydlidar_help.h>

class Motor{
    private:
        int L_RpwmPin; //LEFT WHEEL. direction: FRONT
        int L_LpwmPin; //LEFT WHEEL. direction: BACK
        int R_RpwmPin; //RIGHT WHEEL. direction: FRONT
        int R_LpwmPin; //RIGHT WHEEL. direction: BACK

        // NEED ALWAYS ON : WHEEL MOVE CONTROL INPUT PIN 
        int R_RENPin;
        int R_LENPin;
        int L_RENPin;
        int L_LENPin;


        void motor_setup();

        CYdLidar lidar;
        LaserScan scanData; //스캔 데이터 직접 받는 곳 삭제 절대 XXX
        std::vector<LaserPoint> usableData; //스캔 데이터  정제하여 받는 곳

        void lidar_setup();
        int motor_setup();

        void motor_setup(int lr_pwmPin, int ll_pwmPin,int rr_pwmPin, int rl_pwmPIn, int rr_enPin, int rl_enPin, int lr_enPin, int ll_enPin);
        bool pwm_isvalid(int pwm);

        float calculate_dgrspeed(int pwm);

        float average_pwm(int l_pwm, int r_pwm);

        float calculate_tan(float degree) ;

        void calculate_twin_pwm(float coredistance, int pwm, float degree, int* pwm1, int* pwm2);

        void lmotor_run(int pwm, bool front=true) ;
        void rmotor_run(int pwm, bool front=true) ;

        

    public:
        LaserScan scanData;
        Motor();
        Motor(int lr_pwmPin, int ll_pwmPin,int rr_pwmPin, int rl_pwmPin, int rr_enPin, int rl_enPin, int lr_enPin, int ll_enPin);
        ~Motor();

<<<<<<< HEAD
        std::vector<LaserPoint> set_scanpoints(LaserScan& scan);

        void calculate_ang_dist();
=======
        void scan_oneCycle();
        std::vector<LaserPoint> Motor::get_scanData();
        std::vector<LaserPoint> show_scanData();
        
>>>>>>> 6872a42 (CYdLidar 클래스 넣어서 라이다 초기화 + 설정 + 1회만 가동할 수 있도록 함. 모터 클래스에서 스캔 값 가지고 올 수 있으나, 임계값 이하의 거리 데이터만 따로 필터링해서 모터 동작하도록 해야함.)
        void straight(int pwm);

       // @brief back off
        void backoff(int pwm);

        void stop();

        // @brief rotate by degree : R positive, L negative
        //@brief this function is not depend on time, only whether is degree reach
        void rotate(int pwm, float degree);
        
        // @brief default curve (30 degree)
        void left_curve(bool recover, int pwm);
        // @brief default curve (30 degree)
        void right_curve(bool recover, int pwm);

        // @brief user setting curve ( user setting pwm diffrence provokes curvature )
        void left_curve(bool recover,  int l_pwm, int r_pwm);
        // @brief user setting curve ( user setting pwm diffrence provokes curvature )
        void right_curve(bool recover, int l_pwm, int r_pwm);

        void curve_avoid(float distance, int pwm, float degree, bool recover);
        //@brief this function is not depend on time, only whether is degree reach
        void curve_corner(float connerdistance, int pwm, float degree);
};
