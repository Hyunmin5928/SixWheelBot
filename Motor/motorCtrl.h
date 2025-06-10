#pragma once

#include <wiringPi.h>
#include <softPwm.h>
#include <iostream>
#include <motorCtrl.h>
#include <chrono>
#include <functional>

class Motor{
    private:
        int R_pwmPin;
        int L_pwmPin;
        int RR_ENPin;
        int RL_ENPin;
        int LR_ENPin;
        int LL_ENPin;

        void motor_setup();
        void motor_setup(int r_pwmPin, int l_pwmPin, int rr_enPin, int rl_enPin, int lr_enPin, int ll_enPin)
        bool pwm_isvalid(int pwm);

        //@brief tip : if want to direction backward, input false
        void lmotor_direction_front(bool front);
        //@brief tip : if want to direction backward, input false
        void rmotor_direction_front(bool front);

    public:
        Motor();
        Motor(int r_pwmPin, int l_pwmPin, int rr_enPin, int rl_enPin, int lr_enPin, int ll_enPin);
        ~Motor();

        void straight(int pwm);

       // @brief 후진
       // @brief back off
        void backoff(int pwm);

        // 제자리 회전 : 오른쪽 + 왼쪽 -
        // @brief 해당 각도에 도달할 때까지 자체적으로 딜레이 유지함
        // @brief rotate by degree : R positive, L negative
        void rotate(int pwm, float degree);
        
        // @brief 기본 커브 default curve (30 degree)
        void left_curve(bool recover, int pwm);
        // @brief 기본 커브 default curve (30 degree)
        void right_curve(bool recover, int pwm);

        // @brief 사용자 지정커브 ( 사용자가 세팅한 pwm 값의 차이는 곡률에 영향을 줌 )
        // @brief user setting curve ( user setting pwm diffrence provokes curvature )
        void left_curve(bool recover,  int l_pwm, int r_pwm);
        // @brief 사용자 지정커브 ( 사용자가 세팅한 pwm 값의 차이는 곡률에 영향을 줌 )
        // @brief user setting curve ( user setting pwm diffrence provokes curvature )
        void right_curve(bool recover, int l_pwm, int r_pwm);

        // 커브의 새로운 함수.. 원하는 pwm(오,왼 평균), degree 값으로 
        // 함수 내에서 자동으로 l, r pwm을 설정해주고 이동을 수행해줄 수 있도록..!
        //@brief recover를 true로 지정해야 본래 각도로 되돌아옴
        void curve(bool recover, int pwm, float degree);
        
};
