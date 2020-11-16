
#include "SimpleSPI/SimpleSPI.cpp"


float data;
void setup() {

  Serial.begin(19200);


  
  if(spi.begin())
    Serial.println("MPU9250 set");

  // If initialization fails
  else
    while(true)
      Serial.println("Initialization failed");
      


}

void loop() {



  data = spi.transfer(0x43);

  Serial.print("ACCEL: ");
  Serial.println(spi.returnVar());


}
