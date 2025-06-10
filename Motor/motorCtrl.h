#pragma once

#include <wiringPi.h>
#include <softPwm.h>
#include <iostream>
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
        void motor_setup(int r_pwmPin, int l_pwmPin, int rr_enPin, int rl_enPin, int lr_enPin, int ll_enPin);
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

       // @brief ����
       // @brief back off
        void backoff(int pwm);

        // ���ڸ� ȸ�� : ������ + ���� -
        // @brief �ش� ������ ������ ������ ��ü������ ������ ������
        // @brief rotate by degree : R positive, L negative
        void rotate(int pwm, float degree);
        
        // @brief �⺻ Ŀ�� default curve (30 degree)
        void left_curve(bool recover, int pwm);
        // @brief �⺻ Ŀ�� default curve (30 degree)
        void right_curve(bool recover, int pwm);

        // @brief ����� ����Ŀ�� ( ����ڰ� ������ pwm ���� ���̴� ����� ������ �� )
        // @brief user setting curve ( user setting pwm diffrence provokes curvature )
        void left_curve(bool recover,  int l_pwm, int r_pwm);
        // @brief ����� ����Ŀ�� ( ����ڰ� ������ pwm ���� ���̴� ����� ������ �� )
        // @brief user setting curve ( user setting pwm diffrence provokes curvature )
        void right_curve(bool recover, int l_pwm, int r_pwm);

        // Ŀ���� ���ο� �Լ�.. ���ϴ� pwm(��,�� ���), degree ������ 
        // �Լ� ������ �ڵ����� l, r pwm�� �������ְ� �̵��� �������� �� �ֵ���..!
        //@brief recover�� true�� �����ؾ� ���� ������ �ǵ��ƿ�
        void curve(int pwm, float degree, bool recover);
        
};
