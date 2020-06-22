
int ch3;

void setup() {

  // Set pin 7 as input mode
  pinMode(7, INPUT);

  // Set the data rate in bits/sec
  Serial.begin(9600);

}

void loop() {

  // Read the pulse value of each channel
  ch3 = pulseIn(7, HIGH, 25000);

  
  // Print the value of each channel
  Serial.print("Channel 3:");
  Serial.println(ch3);

  //Delay printing
  delay(100); 
}
