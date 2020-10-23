/*
 *  Motor control with RC receiver by Andres Escobedo
 */


#include <Servo.h>

  // ESC control is the same PWM input as with a servo
  Servo ESC;

  //Throttle on remote
  int ch3;
  
  int InputValue;

void setup() {

  // Set pin 7 as input mode (receiver)
  pinMode(7, INPUT);

  // Set the data rate in bits/sec
  Serial.begin(19200);

  // servo.attach(pin, min, max)
  ESC.attach(9,1000,2000);

}

void loop() {

  // Read the pulse value of each channel
  //ch3 = pulseIn(7, HIGH, 25000);    // 993 to 1986

  // map the value of the input channel 3 to the speed of the motor
  // map(value, fromLow, fromHigh, toLow, toHigh)
  //InputValue = map(ch3, 993, 1986, 0, 180);

  //ESC.write(InputValue);
  
  // Print the value of each channel
  Serial.print("Channel 3:");
  Serial.println(ch3);

  //Delay printing
  delay(100); 
  
}
