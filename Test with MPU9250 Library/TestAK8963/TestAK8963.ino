#include "MPU9250.h"


float hx;

void setup() {
  
  MPU9250 IMU(SPI,10);

  int status;
  status = IMU.begin();

  IMU.readSensor();
  hx = IMU.getAccelX_mss();

  Serial.begin(19200);

}

void loop() {


  Serial.println(hx);

}
