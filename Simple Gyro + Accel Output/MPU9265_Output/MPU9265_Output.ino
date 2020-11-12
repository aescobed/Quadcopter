
#include "SimpleSPI/SimpleSPI.cpp"


float data;
void setup() {

  Serial.begin(19200);
  
  spi.begin();

  //spi.beginTransaction


}

void loop() {



  data = spi.transfer(0x43);

  Serial.print("GYRO: ");
  Serial.println(data);


}
