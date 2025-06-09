#pragma once

class Motor{
    private:
        int pwmPin;
        int R_EN;
        int L_EN;

        void motor_setup();
        bool pwm_isvalid(int pwm);
    public:
        Motor();
        Motor(int pwmPin, int R_EN, int L_EN);
        ~Motor();

        void straight(int pwm, int time);
        void straight(int pwm, bool func);
        void rotate(int pwm, int time);
        
};
