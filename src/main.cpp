#include <Arduino.h>
#include <PDM.h> 
#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>
#include <stdio.h>
#include <stdlib.h>
#include "secrets.h"
// #include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "imufx.h"
#include "poxfx.h"

const char* ssid = SSID;
const char* password = WIFIPASSWORD;


WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

#define HEART_O2_DATA_REPORTING_PERIOD_MS 1000
uint32_t heartO2DataSampleTime;

// Define NTP Client to get time
// WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 3 * 3600; // Offset for 3 hours ahead of UTC
// NTPClient timeClient(ntpUDP);  // NTP server, Time offset in seconds, Update interval in milliseconds

int status = WL_IDLE_STATUS;

const char broker[] = HOST; // IP address of the mqtt host. 

int mqtt_port = PORT;

const char temperature[] = "measurement/temperature"; 
const char sound[] = "measurement/sound";
const char heart[] = "measurement/heart";
const char sleep[] = "measurement/sleep";
const char walking[] = "measurement/walking";
const char jogging[] = "measurement/jogging";
const char steps[] = "measurement/steps";
const char biking[] = "measurement/biking";
const char driving[] = "measurement/driving";
const char idling[] = "measurement/idling";
const char oxygen[] = "measurement/oxygen";

const char topic[] = "test/topic";

char buffer[10];


unsigned long millisCounter = 0;
uint8_t secondsCounter = 0;

bool wifi_connected = false;
bool mqtt_connected = false;
bool subscribed = false;

int last_tiggered_second = -1;
int heart_last_tiggered_second = -1;

int heart_rate = 0;
int oxygen_concentration = 0;
uint16_t last_step_count = 0;

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


void setup() {
    Wire.begin();
    Serial1.begin(115200);

    // Connect to wifi
    if(wifiConnect()){
      wifi_connected = true;
      if(mqttConnect()){
          mqtt_connected = true;
      } 
    }

  // Initialize a NTPClient to get time
  // timeClient.begin();

  imu_mlc_init();
  
  pox_init();
   
  Serial1.println("\nHello world");
  oneSecondRefreshTime = millis();
  activityDataSampleTime = millis();
  stepsDataSampleTime = millis();
  heartO2DataSampleTime = millis();
}

void loop() {

  mqttClient.poll();
  pox.update();
  // timeClient.update();


  // Reconnect with the MQTT broker if not connected.  
  if(!mqttClient.connected()){
    mqttConnect();
  }

  // Refresh the minutes counter after every one second.
  if(millis() - oneSecondRefreshTime >= REPORTING_PERIOD_MS){
    update_timers(REPORTING_PERIOD_MS);
    // display_progress();
    oneSecondRefreshTime = millis();
  }
  
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    heart_rate = (int)pox.getHeartRate();
    oxygen_concentration = (int)pox.getSpO2();
    // Serial1.print(" Heart rate:");
    // Serial1.print(heart_rate);
    // Serial1.print("bpm / SpO2:");
    // Serial1.print(oxygen_concentration);
    // Serial1.print(" \r");
    // display_progress();
    tsLastReport = millis();
  }


  if (mems_event){
    mems_event=0;
    LSM6DSOX_MLC_Status_t mlc_status;
    AccGyr.Get_MLC_Status(&mlc_status);
    LSM6DSOX_Event_Status_t status;
    AccGyr.Get_X_Event_Status(&status);

    if (mlc_status.is_mlc1) {
      uint8_t mlc_out[8];
      AccGyr.Get_MLC_Output(mlc_out);
      getActivitySummary(mlc_out[0]);
    }
      
    if (status.StepStatus){
      // New step detected, so update the step counter
      AccGyr.Get_Step_Count(&step_count);
    }
  }


  // Sending Oxygen concentration and Heart rate after every HEART_O2_DATA_REPORTING_PERIOD_MS (5 secponds)
  if(millis() - heartO2DataSampleTime >= HEART_O2_DATA_REPORTING_PERIOD_MS){
    // Filtering out any oxygen concentration value that is less than 60
    if(oxygen_concentration > 60){
      mqtt_send_int(oxygen, oxygen_concentration);
    }
    
    // Filtering out any heart rate that is less than 50
    if(heart_rate > 50){
      mqtt_send_int(heart, heart_rate);
    }
    heartO2DataSampleTime = millis();
  }


  // Sending the number of steps after every STEPS_REPORTING_PERIOD_MS (5 seconds)
  if(millis() - stepsDataSampleTime >= STEPS_REPORTING_PERIOD_MS){
    // No need to send the current step count unless it is different from the previous one.
    if(step_count !=  last_step_count){
      mqtt_send_int(steps, step_count);
      last_step_count = step_count;
    }
    stepsDataSampleTime = millis();
  }


  // Sending activity data after every ACTIVITY_REPORTING_PERIOD_MS (5 seconds)
  if(millis() - activityDataSampleTime >= ACTIVITY_REPORTING_PERIOD_MS){
    if(state.current == stationary_state){
      mqtt_send_float(idling, activity_minutes.stationary);
    } else if(state.current == walking_state){
      mqtt_send_float(walking, activity_minutes.walking);
    } else if(state.current == jogging_state){
      mqtt_send_float(jogging, activity_minutes.jogging);
    } else if(state.current == biking_state){
      mqtt_send_float(biking, activity_minutes.biking);
    } else if(state.current == driving_state){
      mqtt_send_float(driving, activity_minutes.driving);
    } else if(state.current == unknown_state){
      // mqtt_send_float("others", activity_minutes.unknown); 
    }

    activityDataSampleTime = millis();
  }

  // Sleep tracking
  if(millis() - timer > 1000){ 
    sleep_tracking();
    timer = millis();
  }  

}
