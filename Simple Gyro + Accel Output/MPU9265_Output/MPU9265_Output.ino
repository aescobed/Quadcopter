
#include "SimpleSPI/SimpleSPI.cpp"


float data;
void setup() {

  Serial.begin(19200);


  
  spi.begin();

  

  


}

void loop() {



  data = spi.transfer(0x43);

  Serial.print("ACCEL: ");
  Serial.println(spi.readSensor());


}
