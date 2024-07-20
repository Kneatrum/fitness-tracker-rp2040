#include <Arduino.h>
#include <Arduino_LSM6DSOX.h>
#include <PDM.h> 
#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>
#include <stdio.h>
#include <stdlib.h>
#include "secrets.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

const char* ssid = SSID;
const char* password = WIFIPASSWORD;


WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

PulseOximeter pox;
uint32_t tsLastReport = 0;

// Define NTP Client to get time
WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 3 * 3600; // Offset for 3 hours ahead of UTC
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds, 60000);  // NTP server, Time offset in seconds, Update interval in milliseconds

int status = WL_IDLE_STATUS;

const char broker[] = "192.168.100.10"; // IP address of the mqtt host. 

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

int last_tiggered_second = -1;

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


bool time_to_print = false;

void onBeatDetected()
{
  time_to_print = true;
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

  // Initialize a NTPClient to get time
  timeClient.begin();

  if(mqttConnect()){
    mqtt_connected = true;
  } 

  while (!pox.begin()) {
    Serial1.print("Initializing pulse oximeter..");
    delay(1000);
  }

  pox.setOnBeatDetectedCallback(onBeatDetected);
   
}

void loop() {

  mqttClient.poll();
  timeClient.update();
  pox.update();

 
  if(!mqttClient.connected()){
    mqttConnect();
  }

  int currentSecond = timeClient.getSeconds();

  // publish a message roughly every second.
  if( currentSecond % 5 == 0 && last_tiggered_second != currentSecond){
    if(IMU.temperatureAvailable()){
      float temperature_float = 0;
      IMU.readTemperatureFloat(temperature_float);

      mqttClient.beginMessage(temperature);
      // mqttClient.print("Temperature is ");
      sprintf(buffer, "%.2f", temperature_float);  // 2 decimal places
      mqttClient.print(buffer);
      // mqttClient.println(" Degrees");
      // mqttClient.print("Time is ");
      // mqttClient.println(timeClient.getFormattedTime());
      mqttClient.endMessage();
    }
    last_tiggered_second =  currentSecond;
  }

  int heartCurrentSecond = timeClient.getSeconds();
  if( heartCurrentSecond % 5 == 0 && heart_last_tiggered_second != heartCurrentSecond && time_to_print){
    // Serial1.print(pox.getSpO2());
    mqttClient.beginMessage(heart);
    float result = pox.getHeartRate();
    sprintf(buffer, "%.2f", result);  // 2 decimal places
    mqttClient.print(buffer);
    mqttClient.endMessage();
    heart_last_tiggered_second = heartCurrentSecond;
    time_to_print = false;
  }

}
