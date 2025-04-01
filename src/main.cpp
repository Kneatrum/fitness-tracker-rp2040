#include <Arduino.h>
#include <PDM.h> 
#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>
#include <stdio.h>
#include <stdlib.h>
#include "secrets.h"
#include <WiFiUdp.h>
#include <Wire.h>
#include <DS3231.h>
#include "imufx.h"
#include "poxfx.h"
#include <math.h> 


DS3231 myRTC;
bool century = false;
bool h12Flag;
bool pmFlag;
byte alarmDay, alarmHour, alarmMinute, alarmSecond, alarmBits;
bool alarmDy, alarmH12Flag, alarmPmFlag;
int lastMinute = -1;
unsigned long previousMillis = 0;  // Stores the last time the calculation was done
double sample_accumulator = 0;
uint8_t sample_counter = 0;
bool second_average_data_ready = false;
const long interval = 40;  // 40 milliseconds interval for 25 Hz

byte year;
byte month;
byte date;
byte dOW;
byte hour;
byte minute;
byte second;



volatile bool ready_to_print = false;
volatile double result = 0;
volatile int32_t ax, ay, az;
uint8_t counter = 0;

#define USING_MBED_RPI_PICO_TIMER_INTERRUPT        true

// These define's must be placed at the beginning before #include "TimerInterrupt_Generic.h"
// _TIMERINTERRUPT_LOGLEVEL_ from 0 to 4
#define _TIMERINTERRUPT_LOGLEVEL_     4

// To be included only in main(), .ino with setup() to avoid `Multiple Definitions` Linker Error
#include "MBED_RPi_Pico_TimerInterrupt.h"



// Never use Serial.print inside this mbed ISR. Will hang the system
void TimerHandler0(uint alarm_num)
{ 
  static bool toggle0 = false;

  ///////////////////////////////////////////////////////////
  // Always call this for MBED RP2040 before processing ISR
  TIMER_ISR_START(alarm_num);
  ///////////////////////////////////////////////////////////

  //timer interrupt toggles pin LED_BUILTIN
  digitalWrite(LED_BUILTIN, toggle0);
  // toggle0 = !toggle0;
  ready_to_print = true;

 
    
    
    
    
 
    // sample_accumulator += sqrt((ax * ax) + (ay * ay) + (az * az));
    // if(sample_counter == 25){
    //   result = sample_accumulator/25;
    //   sample_counter = 0;
    //   second_average_data_ready = true;
    //   sample_accumulator = 0;
    // }
    // sample_counter++;
    
  






  ////////////////////////////////////////////////////////////
  // Always call this for MBED RP2040 after processing ISR
  TIMER_ISR_END(alarm_num);
  ////////////////////////////////////////////////////////////
}


#define TIMER0_INTERVAL_MS        40 // Equivalent to 25Hz

// Init MBED_RPI_PICO_Timer, can use any from 0-15 pseudo-hardware timers
MBED_RPI_PICO_Timer ITimer0(0);
// MBED_RPI_PICO_Timer ITimer1(1);

bool show_difference = false;
unsigned long lastTimestamp  = 0;

const char* ssid = SSID;
const char* password = WIFIPASSWORD;


WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

#define HEART_O2_DATA_REPORTING_PERIOD_MS 5000
uint32_t heartO2DataSampleTime;


const long utcOffsetInSeconds = 3 * 3600; // Offset for 3 hours ahead of UTC

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
const char acceleration[] = "measurement/acceleration";
const char angular_velocity[] = "measurement/angularvelocity";
const char movement_magnitude[] = "measurement/movementmagnitude";
int32_t imuTimeCounter = 0;
bool toggleState = false;

const char topic[] = "test/topic";

char buffer[10];


unsigned long millisCounter = 0;
uint8_t secondsCounter = 0;

bool wifi_connected = false;
bool mqtt_connected = false;
bool subscribed = false;

int last_tiggered_second = -1;
int heart_last_tiggered_second = -1;

float heart_rate = 0;
float heart_rate_samples = 0;
uint16_t heart_samples = 299;
uint16_t heart_sample_count = 0;
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


void getDateStuff(byte& year, byte& month, byte& date, byte& dOW,
                  byte& hour, byte& minute, byte& second) {
    // Call this if you notice something coming in on
    // the serial port. The stuff coming in should be in
    // the order YYMMDDwHHMMSS, with an 'x' at the end.
    boolean gotString = false;
    char inChar;
    byte temp1, temp2;
    char inString[20];
    
    byte j=0;
    while (!gotString) {
        if (Serial1.available()) {
            inChar = Serial1.read();
            inString[j] = inChar;
            j += 1;
            if (inChar == 'x') {
                gotString = true;
            }
        }
    }
    Serial1.println(inString);
    // Read year first
    temp1 = (byte)inString[0] -48;
    temp2 = (byte)inString[1] -48;
    year = temp1*10 + temp2;
    // now month
    temp1 = (byte)inString[2] -48;
    temp2 = (byte)inString[3] -48;
    month = temp1*10 + temp2;
    // now date
    temp1 = (byte)inString[4] -48;
    temp2 = (byte)inString[5] -48;
    date = temp1*10 + temp2;
    // now Day of Week
    dOW = (byte)inString[6] - 48;
    // now hour
    temp1 = (byte)inString[7] -48;
    temp2 = (byte)inString[8] -48;
    hour = temp1*10 + temp2;
    // now minute
    temp1 = (byte)inString[9] -48;
    temp2 = (byte)inString[10] -48;
    minute = temp1*10 + temp2;
    // now second
    temp1 = (byte)inString[11] -48;
    temp2 = (byte)inString[12] -48;
    second = temp1*10 + temp2;
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


  imu_mlc_init();
  
  pox_init();
   
  Serial1.println("\nHello world");
  oneSecondRefreshTime = millis();
  activityDataSampleTime = millis();
  stepsDataSampleTime = millis();
  heartO2DataSampleTime = millis();
  imuTimeCounter = millis();
  previousMillis = millis();

  pinMode(LED_BUILTIN, OUTPUT);
  
  // Interval in microsecs
  if (ITimer0.attachInterruptInterval(TIMER0_INTERVAL_MS * 1000, TimerHandler0))
  Serial1.print(F("Starting ITimer0 OK "));
  else Serial1.println(F("Can't set ITimer0. Select another freq. or timer"));

 
  lastTimestamp = millis();
}

void loop() {

  mqttClient.poll();
  pox.update();

      if (Serial1.available()) {
        getDateStuff(year, month, date, dOW, hour, minute, second);
        
        myRTC.setClockMode(false);  // set to 24h
        //setClockMode(true); // set to 12h
        
        myRTC.setYear(year);
        myRTC.setMonth(month);
        myRTC.setDate(date);
        myRTC.setDoW(dOW);
        myRTC.setHour(hour);
        myRTC.setMinute(minute);
        myRTC.setSecond(second);
        
        // Test of alarm functions
        // set A1 to one minute past the time we just set the clock
        // on current day of week.
        myRTC.setA1Time(dOW, hour, minute+1, second, 0x0, true,
                        false, false);
        // set A2 to two minutes past, on current day of month.
        myRTC.setA2Time(date, hour, minute+2, 0x0, false, false,
                        false);
        // Turn on both alarms, with external interrupt
        myRTC.turnOnAlarm(1);
        myRTC.turnOnAlarm(2);
        
    }


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
    // if( heart_sample_count == heart_samples){
    //   heart_rate = heart_rate_samples/heart_samples;
    //   heart_sample_count = 0;
    // }
    oxygen_concentration = (int)pox.getSpO2();
    tsLastReport = millis();
    heart_sample_count++;
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



  
  int currentMinute = myRTC.getMinute();
  
  // Check if the current second is divisible by 10 and has changed since the last time
  if (currentMinute % 5 == 0 && currentMinute != lastMinute) {
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

    if(step_count !=  last_step_count){
      mqtt_send_int(steps, step_count);
      last_step_count = step_count;
    }

    if(oxygen_concentration >= 60 && oxygen_concentration <= 100){
      mqtt_send_int(oxygen, oxygen_concentration);
    }

    if(heart_rate > 50 && heart_rate <= 170){
      mqtt_send_int(heart, heart_rate);
    }

    // mqtt_send_int(sleep, 3);

    
    lastMinute = currentMinute;
  }


if(millis() - imuTimeCounter >= REPORTING_PERIOD_MS){
  int32_t gx, gy, gz, ax, ay, az;
  int32_t gyroscope[3];
  AccGyr.Get_G_Axes(gyroscope);
  gx = gyroscope[0];
  gy = gyroscope[1];
  gz = gyroscope[2];

  int32_t accelerometer[3];
  AccGyr.Get_X_Axes(accelerometer); 
  ax = accelerometer[0];
  ay = accelerometer[1];
  az = accelerometer[2];

  imuTimeCounter = millis();
  toggleState = !toggleState;
  digitalWrite(LED_BUILTIN, toggleState);
}

}
