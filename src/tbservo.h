#include <Arduino.h>
#include <ESP32Servo.h>
#include "config.h"

enum tbservostatus_enum {
    tb_idle = 0,
    tb_target = 1,
    tb_rest = 2
};

class TBServo {

    public:
        Servo * servo;
        
    private:
        tbservostatus_enum tbservostatus = tb_idle;    
        int servoPin;
        int idlePosition;
        int targetPosition;
        int restPosition;
        unsigned long lastMoveTime;
        unsigned long targetDelay;
        unsigned long restDelay;

    public:
        TBServo(int pin);

        void targetTo(int target_position, int rest_position, int target_delay, int rest_delay);
        void targetTo(bell_struct * bell);
        void update();    
};
