
/* 
 *  Code from https://howtomechatronics.com/
 *  
 *  
 */

#include <Servo.h>

// ESC control is the same PWM input as with a servo
 Servo ESC;

// Potentiometer
 int potValue;

void setup() {

  // servo.attach(pin, min, max)
  ESC.attach(9,1000,2000);

}

void loop() {

  potValue = analogRead(A0);

  // map(value, fromLow, fromHigh, toLow, toHigh)
  potValue = map(potValue, 0, 1023, 0, 180);

  ESC.write(potValue);

}
