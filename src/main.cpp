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
// #include "imufx.h"

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

const char broker[] = "192.168.101.47"; // IP address of the mqtt host. 

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
int heart_last_tiggered_second = -1;


// uint16_t step_count = 0;
char report[256];
uint32_t previous_tick;


void mqtt_send_int(const char topic[], int data){
  mqttClient.beginMessage(topic);
  mqttClient.print(data);
  mqttClient.endMessage();
}

void mqtt_send_str(const char topic[], char* data){
  mqttClient.beginMessage(topic);
  mqttClient.print(data);
  mqttClient.endMessage();
}

void mqtt_send_float(const char topic[], float data){
  char buffer[10];
  mqttClient.beginMessage(topic);
  sprintf(buffer, "%.2f", data);  // 2 decimal places
  mqttClient.print(buffer);
  mqttClient.endMessage();
}

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
  uint8_t count = 0;
  Serial1.print("Connecting to WIFI");
  while(status != WL_CONNECTED){
    status = WiFi.begin(ssid, password);
    delay(1000);
    count++;
    Serial1.print(".");
    
    if(count == 10){
      Serial1.println("\nFailed to connect to WIFI");
      return false;
    }
  }
  Serial1.println();
  Serial1.println("Connected to WIFI");
  return true;
}




bool mqttConnect() {
  uint8_t count = 1;
  Serial1.println("\nConnecting to MQTT broker");
  while (!mqttClient.connect(broker, mqtt_port)) {
    Serial1.print("Connecttion attempt ");
    Serial1.print(count);
    Serial1.print(" Error code = ");
    Serial1.println(mqttClient.connectError());
    count++;
    if(count == 5){
      Serial1.println("MQTT connection failed!");
      return false;
    }
  }

  Serial1.println("You're connected to the MQTT broker!");
  return true;
  
}


bool time_to_print = false;

void onBeatDetected()
{
  time_to_print = true;
}

void setup() {
    // Wire.begin();
    Serial1.begin(115200);
    pinInit();

    // while(!Serial);
    // if(!IMU.begin()){
    //     Serial1.println("Failed to initialize IMU!");
    //     while (1);
    // }

    // Connect to wifi
    if(wifiConnect()){
      wifi_connected = true;
      if(mqttConnect()){
          mqtt_connected = true;
      } 
    }

    // Initialize a NTPClient to get time
    timeClient.begin();


    while (!pox.begin()) {
        Serial1.print("Initializing pulse oximeter..");
        delay(1000);
    }

  pox.setOnBeatDetectedCallback(onBeatDetected);
   
  //  imu_mlc_init();
  Serial1.println("\nHello world");
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
    // if(IMU.temperatureAvailable()){
    //   float temperature_float = 0;
    //   IMU.readTemperatureFloat(temperature_float);

    //   mqttClient.beginMessage(temperature);
    //   sprintf(buffer, "%.2f", temperature_float);  // 2 decimal places
    //   mqttClient.print(buffer);
    //   mqttClient.endMessage();
    // }
    Serial1.println("Alive");
    last_tiggered_second =  currentSecond;
  }

  int heartCurrentSecond = timeClient.getSeconds();
  if( heartCurrentSecond % 2 == 0 && heart_last_tiggered_second != heartCurrentSecond && time_to_print){
    Serial1.print(pox.getSpO2());
    mqttClient.beginMessage(heart);
    float result = pox.getHeartRate();
    Serial1.print("Heart rate = ");
    Serial1.println(result);
    sprintf(buffer, "%.2f", result);  // 2 decimal places
    mqttClient.print(buffer);
    mqttClient.endMessage();
    heart_last_tiggered_second = heartCurrentSecond;
    time_to_print = false;
  }

// if (mems_event)
//   {
//     mems_event=0;
//     LSM6DSOX_MLC_Status_t mlc_status;
//     AccGyr.Get_MLC_Status(&mlc_status);
//     LSM6DSOX_Event_Status_t status;
//     AccGyr.Get_X_Event_Status(&status);

//     if (mlc_status.is_mlc1) {
//       uint8_t mlc_out[8];
//       AccGyr.Get_MLC_Output(mlc_out);
//       printMLCStatus(mlc_out[0]);
//     }
    
//     if (status.StepStatus)
//     {
//       // New step detected, so print the step counter
//       AccGyr.Get_Step_Count(&step_count);
//     }
//   }
  
//   // Print the step counter in any case every 3000 ms
//   uint32_t current_tick = millis();
//   if((current_tick - previous_tick) >= 3000)
//   {
//     snprintf(report, sizeof(report), "#Step counter: %d", step_count);
//     Serial1.println(report);
//     mqtt_send_str(steps, report);
//     previous_tick = millis();
//   }

}
