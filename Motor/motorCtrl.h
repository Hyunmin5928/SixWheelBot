#pragma once

class Motor{
    private:
        int L_RpwmPin;
        int L_LpwmPin;
        int R_RpwmPin;
        int R_LpwmPin;

        int R_RENPin;
        int R_LENPin;
        int L_RENPin;
        int L_LENPin;

        void motor_setup();
        void motor_setup(int lr_pwmPin, int ll_pwmPin,int rr_pwmPin, int rl_pwmPIn, int rr_enPin, int rl_enPin, int lr_enPin, int ll_enPin);
        bool pwm_isvalid(int pwm);

        //@brief tip : if want to direction backward, input false
        //@breif prevent hardware damage from pwm collison
        void lmotor_direction_front(bool front);
        //@brief tip : if want to direction backward, input false
        //@breif prevent hardware damage from pwm collison
        void rmotor_direction_front(bool front);

        float calculate_dgrspeed(int pwm);

        float average_pwm(int l_pwm, int r_pwm);

        void only_lmotor_run(int pwm, bool front=true) ;
        void only_rmotor_run(int pwm, bool front=true) ;

    public:
        Motor();
        Motor(int lr_pwmPin, int ll_pwmPin,int rr_pwmPin, int rl_pwmPin, int rr_enPin, int rl_enPin, int lr_enPin, int ll_enPin);
        ~Motor();

        void straight(int pwm);

       // @brief back off
        void backoff(int pwm);

        void stop();

        // @brief rotate by degree : R positive, L negative
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
        void curve_coner(int pwm, float degree);
};
