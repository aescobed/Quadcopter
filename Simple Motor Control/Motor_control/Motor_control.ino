
/* 
 *  Code from https://howtomechatronics.com/
 *  
 *  
 */

 #include <servo.h>

// ESC control is the same PWM input as with a servo
 Servo ESC;

// Potentiometer
 int potValue

void setup() {

  ESC.attach(9,1000,2000);

}

void loop() {

  potValue = analogRead(A0);

  potValue = map(potValue, 0, 1023, 0, 180);

  ESC.write(potValue);

}
