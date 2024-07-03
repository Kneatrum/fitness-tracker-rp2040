#include <Arduino.h>
#include <Arduino_LSM6DSOX.h>
#include <PDM.h> 
#include <SPI.h>
#include <WiFiNINA.h>


// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
  Serial.print(result);
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}