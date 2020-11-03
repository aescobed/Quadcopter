
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

  int throttleHigh;
  int throttleLow;
  int yawHigh;
  int yawLow;

void setup() {

  // Set pin 7 as input mode for throttle (receiver)
  pinMode(7, INPUT);
  pinMode(6, INPUT);

  // Set the data rate in bits/sec for the serial monitor
  Serial.begin(19200);

  getRanges();

  // servo.attach(pin, min, max)
  //ESC.attach(9,1000,2000);

}

void loop() {

  // Read the pulse value of each channel
  throttle = pulseIn(7, HIGH, 250000);   
  yaw = pulseIn(6, HIGH, 250000);    
  // map the value of the input channel 3 to the speed of the motor
  // map(value, fromLow, fromHigh, toLow, toHigh)
  //throttle = map(throttle, 993, 1986, 0, 180);
  //yaw = map(yaw, 993, 1986, 0, 180);
  
  //ESC.write(InputValue);


  // Print the value of each channel
  Serial.print("Motor throttle:");
  Serial.print(throttle);
  Serial.print("\t\t");
  Serial.print("Motor yaw:");
  Serial.println(yaw);

  //delay(500);

}

// Finds and sets the values ranges for each channel
void getRanges()
{
    
    int iter = 0;
    
    throttleHigh = 0;
    throttleLow = 9999;
    yawHigh = 0;
    yawLow = 9999;

    Serial.println("Setting channel boundaries...");

    while(iter < 200){
      throttle = pulseIn(7, HIGH, 250000);   
      yaw = pulseIn(6, HIGH, 250000);    
  

      if(throttle > throttleHigh)
        throttleHigh = throttle;

      if(throttle < throttleLow)
        throttleLow = throttle;

      if(yaw > yawHigh)
        yawHigh = yaw;

      if(yaw < yawLow)
        yawLow = yaw;
  
      iter++;
      
    }

    Serial.print("throttle: ");
    Serial.print("\t\t");
    Serial.print(throttleLow);
    Serial.print("-");
    Serial.println(throttleHigh);

    Serial.print("yaw: ");
    Serial.print("\t\t");
    Serial.print(yawLow);
    Serial.print("-");
    Serial.println(yawHigh);
    
    
}







/*
  wiring_pulse.c - pulseIn() function
  Part of Arduino - http://www.arduino.cc/

  Copyright (c) 2005-2006 David A. Mellis

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA

  $Id: wiring.c 248 2007-02-03 15:36:30Z mellis $
*/

#include "wiring_private.h"
#include "pins_arduino.h"

/* Measures the length (in microseconds) of a pulse on the pin; state is HIGH
* or LOW, the type of pulse to measure.  Works on pulses from 2-3 microseconds
* to 3 minutes in length, but must be called at least a few dozen microseconds
* before the start of the pulse. */

unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout)
{
  // cache the port and bit of the pin in order to speed up the
  // pulse width measuring loop and achieve finer resolution.  calling
  // digitalRead() instead yields much coarser resolution.
  uint8_t bit = digitalPinToBitMask(pin);
  uint8_t port = digitalPinToPort(pin);
  uint8_t stateMask = (state ? bit : 0);
  unsigned long width = 0; // keep initialization out of time critical area
  
  // convert the timeout from microseconds to a number of times through
  // the initial loop; it takes 16 clock cycles per iteration.
  unsigned long numloops = 0;
  unsigned long maxloops = microsecondsToClockCycles(timeout) / 16;
  
  // wait for any previous pulse to end
  while ((*portInputRegister(port) & bit) == stateMask)
    if (numloops++ == maxloops)
      return 0;
  
  // wait for the pulse to start
  while ((*portInputRegister(port) & bit) != stateMask)
    if (numloops++ == maxloops)
      return 0;
  
  // wait for the pulse to stop
  while ((*portInputRegister(port) & bit) == stateMask) {
    if (numloops++ == maxloops)
      return 0;
    width++;
  }

  // convert the reading to microseconds. The loop has been determined
  // to be 20 clock cycles long and have about 16 clocks between the edge
  // and the start of the loop. There will be some error introduced by
  // the interrupt handlers.
  return clockCyclesToMicroseconds(width * 21 + 16);
}
