#include <Arduino.h>
#include <tbservo.h>

    TBServo::TBServo(int pin) {
        // Initialize servo object
        targetPosition = 0;
        lastMoveTime = 0;
        servo = new Servo();
        servo->attach(pin, 500, 2500); // attaches the servo on pin 18 to the servo object
	    servo->setPeriodHertz(200);    // standard 50 hz servo
        
	}

    void TBServo::targetTo(int target_position, int rest_position, int target_delay, int rest_delay) {
        targetPosition = target_position;
        restPosition = rest_position;
        targetDelay = target_delay;
        restDelay = rest_delay;
        servo->write(targetPosition);         // move to the target position
        lastMoveTime = millis();
        tbservostatus = tb_target;
    }

    void TBServo::targetTo(bell_struct * bs) {
        targetPosition = bs->target;
        restPosition = bs->rest;
        targetDelay = bs->target_time_ms;
        restDelay = bs->rest_time_ms;
        servo->write(targetPosition);         // move to the target position
        lastMoveTime = millis();
        tbservostatus = tb_target;
    }


    void TBServo::update() {

        switch(tbservostatus){
            case tb_idle :
               //servo->release();  // set PWM to 0% dutycycle (off)
            break;

            case tb_target :
                if (millis() > lastMoveTime + targetDelay) {
                    tbservostatus = tb_rest;
                    lastMoveTime = millis();
                    servo->write(restPosition);
                    //Serial.printf("%lu status from target to rest\r\n", lastMoveTime);
                }
            break;
            case tb_rest :
                if (millis() > lastMoveTime + restDelay) {
                    tbservostatus = tb_idle;
                    lastMoveTime = millis();
                    //Serial.printf("%lu status from rest to idle\r\n",lastMoveTime);
                }
            break;
        }
    }

