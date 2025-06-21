#pragma once
#include "../../Main/LiDAR/Lidar.h"
#include "../../Main/LiDAR/YDLidar/YDLidar-SDK/src/CYdLidar.h"
#include <wiringPi.h>
#include <softPwm.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <functional>
#include <core/common/ydlidar_help.h>
#include <fstream>
#include <ctime>
#include <string>
#include <../logger.h>

using json = nlohmann::json;
extern int         LOG_LEVEL;
extern std::atomic<bool> running;

using util::Logger;
using util::LogLevel;


class Motor{
    private:
        std::string SERVER_IP;
        int         SERVER_PORT;
        std::string CLIENT_IP;
        int         CLIENT_PORT;
        std::string ALLOW_IP;
        double      ACK_TIMEOUT;
        int         RETRY_LIMIT;
        std::string LOG_FILE="Motor/log/motor_log.txt"; // 기본 로그 파일 경로

        int log_fd;
        int sock_fd = -1;


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
        void motor_setup(int lr_pwmPin, int ll_pwmPin,int rr_pwmPin, int rl_pwmPIn, int rr_enPin, int rl_enPin, int lr_enPin, int ll_enPin);
        bool pwm_isvalid(int pwm);

        float calculate_dgrspeed(int pwm);

        float average_pwm(int l_pwm, int r_pwm);

        float calculate_tan(float degree) ;

        void calculate_twin_pwm(float coredistance, int pwm, float degree, int* pwm1, int* pwm2);

        void lmotor_run(int pwm, bool front) ;
        void rmotor_run(int pwm, bool front) ;

    public:
        Motor();
        Motor(int lr_pwmPin, int ll_pwmPin,int rr_pwmPin, int rl_pwmPin, int rr_enPin, int rl_enPin, int lr_enPin, int ll_enPin);
        ~Motor();
        
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