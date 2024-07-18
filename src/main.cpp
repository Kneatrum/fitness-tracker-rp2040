#include <Arduino.h>
#include <Arduino_LSM6DSOX.h>
#include <PDM.h> 
#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>
#include <stdio.h>
#include <stdlib.h>
#include "secrets.h"

const char* ssid = SSID;
const char* password = WIFIPASSWORD;


WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

int status = WL_IDLE_STATUS;

const char broker[] = "192.168.100.10"; // IP address of the mqtt host
int mqtt_port = 1883;

const char temperature[] = "measurement/temperature"; 
const char sound[] = "measurement/sound";
const char heart[] = "measurement/heart";
const char sleep[] = "measurement/sleep";
const char walking[] = "measurement/walking";
const char jogging[] = "measurement/jogging";
const char steps[] = "measurement/steps";
const char biking[] = "measurement/biking";
const char idling[] = "measurement/idling";

const char topic[] = "test/topic";

char buffer[10];


unsigned long millisCounter = 0;
uint8_t secondsCounter = 0;

bool wifi_connected = false;
bool mqtt_connected = false;
bool subscribed = false;

void pinInit(){
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);

  digitalWrite(LEDR, LOW);
  digitalWrite(LEDG, LOW);
  digitalWrite(LEDB, LOW);
}

bool wifiConnect(){
  status = WiFi.begin(ssid, password);
  if ( status != WL_CONNECTED) return false;
  return true; 
}




bool mqttConnect() {
  // Serial.print("checking wifi...");
  // while (WiFi.status() != WL_CONNECTED) {
  //   Serial.print(".");
  //   delay(1000);
  // }

  Serial.print("\nconnecting...");
  if (!mqttClient.connect(broker, mqtt_port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    return false;
  }

  Serial.println("You're connected to the MQTT broker!");
  return true;
  
}

void setup() {
  Serial.begin(9600);
  delay(100);
  pinInit();

  // while(!Serial);
  if(!IMU.begin()){
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  // Connect to wifi
  if(wifiConnect()){
    wifi_connected = true;
  } 

  if(mqttConnect()){
    mqtt_connected = true;
  } 
   

}

void loop() {

  mqttClient.poll();

  if(wifi_connected){
    digitalWrite(LEDG, HIGH);
    delay(100);
    digitalWrite(LEDG, LOW);
    delay(1000);
  }


  if(mqtt_connected){
    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDG, HIGH);
    digitalWrite(LEDB, HIGH);
    delay(100);
    digitalWrite(LEDR, LOW);
    digitalWrite(LEDG, LOW);
    digitalWrite(LEDB, LOW);
    delay(1000);
  }


  if(!mqttClient.connected()){
    mqttConnect();
  }

  // publish a message roughly every second.
  if (millis() - millisCounter >= 1000) {
    millisCounter = millis();
    secondsCounter++;
    if(IMU.temperatureAvailable()){
      // int temperature_int = 0;
      float temperature_float = 0;
      // IMU.readTemperature(temperature_int);
      IMU.readTemperatureFloat(temperature_float);

      mqttClient.beginMessage(topic);
      mqttClient.print("Temperature is ");
      sprintf(buffer, "%.2f", temperature_float);  // 2 decimal places
      mqttClient.print(buffer);
      mqttClient.print(" Degrees");
      mqttClient.endMessage();
    }
  }
}
