
/* 
 *  Code from https://howtomechatronics.com/
 *  
 *  
 */

#include <Servo.h>

  // ESC control is the same PWM input as with a servo
  Servo ESC;

  //Throttle channel on remote
  int throttle;
  int yaw;
  
  int InputValue;


void setup() {

  // Set pin 7 as input mode for throttle (receiver)
  pinMode(7, INPUT);
  pinMode(8, INPUT);

  // Set the data rate in bits/sec
  Serial.begin(19200);

  // servo.attach(pin, min, max)
  ESC.attach(9,1000,2000);

}

void loop() {

  // Read the pulse value of each channel
  //throttle = pulseIn(7, HIGH, 25000);   
  yaw = pulseIn(8, HIGH, 25000);    
  // map the value of the input channel 3 to the speed of the motor
  // map(value, fromLow, fromHigh, toLow, toHigh)
  //throttle = map(throttle, 993, 1986, 0, 180);
  yaw = map(yaw, 993, 1986, 0, 180);
  
  //ESC.write(InputValue);


  // Print the value of each channel
  Serial.print("Motor throttle:");
  //Serial.println(throttle);
  Serial.print("Motor yaw:");
  Serial.println(yaw);

  delay(500);

}
