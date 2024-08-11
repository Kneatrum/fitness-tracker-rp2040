#include "imufx.h"

LSM6DSOXSensor AccGyr(&DEV_I2C, LSM6DSOX_I2C_ADD_L);

int32_t LineCounter;
int32_t TotalNumberOfLine;

uint32_t stepsDataSampleTime = 0;
uint32_t activityDataSampleTime = 0;
uint32_t oneSecondRefreshTime = 0;

volatile int mems_event = 0;
uint16_t step_count = 0;


ucf_line_t *ProgramPointer;


bool load_mlc_program(){
  ProgramPointer = (ucf_line_t *)lsm6dsox_activity_recognition_for_mobile;
  TotalNumberOfLine = sizeof(lsm6dsox_activity_recognition_for_mobile) / sizeof(ucf_line_t);
  Serial1.println("Activity Recognition for LSM6DSOX MLC");
  Serial1.print("UCF Number Line=");
  Serial1.println(TotalNumberOfLine);

  for (LineCounter=0; LineCounter<TotalNumberOfLine; LineCounter++) {
    if(AccGyr.Write_Reg(ProgramPointer[LineCounter].address, ProgramPointer[LineCounter].data)) {
      Serial1.print("Error loading the Program to LSM6DSOX at line: ");
      Serial1.println(LineCounter);
      return false;
    }
  }
  return true;
}

 unsigned long INVALID_TIMESTAMP = 4294967295;
 uint8_t INITIALIZING_STATE = 13;
 uint8_t stationary_state = 0;
 uint8_t walking_state = 1;
 uint8_t jogging_state = 4;
 uint8_t biking_state = 8;
 uint8_t driving_state = 12;
 uint8_t sleeping_state = 16;
 uint8_t unknown_state = 20;

 const char* activity_names[14] = {
  "Stationary", // 0
  "Walking",    // 1
  NULL,         // 2
  NULL,         // 3
  "Jogging",    // 4
  NULL,         // 5
  NULL,         // 6
  NULL,         // 7
  "Biking",     // 8
  NULL,         // 9
  NULL,         // 10
  NULL,         // 11
  "Driving",    // 12
  "Initializing" // 13
  };


MinutesTracker InitializeMinutesTracker(){
  MinutesTracker mt;
  mt.stationary = 0.0;
  mt.walking = 0.0;
  mt.jogging = 0.0;
  mt.biking = 0.0;
  mt.driving = 0.0;
  mt.sleeping = 0.0;
  mt.unknown = 0.0;
  return mt;
}

Switch InitializeSwitch(){
  Switch sw;
  sw.current = INITIALIZING_STATE;
  sw.previous = INITIALIZING_STATE;
  sw.current_timestamp = INVALID_TIMESTAMP;
  sw.previous_timestamp = INVALID_TIMESTAMP;
  sw.prev_state_time_taken = 0;
  return sw;
}

Switch state = InitializeSwitch();
MinutesTracker activity_minutes = InitializeMinutesTracker();

void getActivitySummary(uint8_t status){
  state.previous = state.current;
  state.current = status;
  state.previous_timestamp = state.current_timestamp;
  state.current_timestamp = millis();


  // Serial1.print("Previous state: ");
  // Serial1.print(state.previous);
  // Serial1.print(" Current state: ");
  // Serial1.println(state.current);
  // Serial1.print("Previous timestamp: ");
  // Serial1.print(state.previous_timestamp);
  // Serial1.print(" Current timestamp: ");
  // Serial1.println(state.current_timestamp);

  if(state.previous == stationary_state){
    activity_minutes.stationary += (millis() - state.previous_timestamp) / 60000UL;
    state.prev_state_time_taken = activity_minutes.stationary;
  } else if(state.previous == walking_state){
    activity_minutes.walking += (millis() - state.previous_timestamp) / 60000UL;
    state.prev_state_time_taken = activity_minutes.walking;
  } else if(state.previous == jogging_state){
    activity_minutes.jogging += (millis() - state.previous_timestamp) / 60000UL;
    state.prev_state_time_taken = activity_minutes.jogging;
  } else if(state.previous == biking_state){
    activity_minutes.biking += (millis() - state.previous_timestamp) / 60000UL;
    state.prev_state_time_taken = activity_minutes.biking;
  } else if(state.previous == driving_state){
    activity_minutes.driving += (millis() - state.previous_timestamp) / 60000UL;
    state.prev_state_time_taken = activity_minutes.driving;
  } else if(state.previous == unknown_state){
    activity_minutes.unknown += (millis() - state.previous_timestamp) / 60000UL;
    state.prev_state_time_taken = activity_minutes.unknown;
  }
  
  Serial1.println("_____________TRANSITION_______________");
  Serial1.print("Current activity: "); 
  Serial1.print(activity_names[status]);
  Serial1.println();

  Serial1.print("Previous activity: ");
  Serial1.print(activity_names[state.previous]);
  Serial1.println();

  Serial1.print("Time taken: ");
  Serial1.print(state.prev_state_time_taken);
  Serial1.println(" Minutes.");
  Serial1.println("______________________________________");

  // Serial1.println();

}

void INT1Event_cb() {
  mems_event = 1;
}

void imu_mlc_init(){
   uint8_t mlc_out[8];
    //Interrupts.
    attachInterrupt(INT_1, INT1Event_cb, RISING);

    AccGyr.begin();
    AccGyr.Enable_X();
    AccGyr.Enable_G();

    if(load_mlc_program()){
        Serial1.println("Program loaded inside the LSM6DSOX MLC");
    }
    AccGyr.Enable_Pedometer();

    delay(3000);

    AccGyr.Get_MLC_Output(mlc_out);
    getActivitySummary(mlc_out[0]);

}

void display_progress(){
   if(state.current == stationary_state){
    Serial1.println("_____STATIONARY_____");
    Serial1.print("Current timestamp: ");
    Serial1.println(state.current_timestamp);

    Serial1.print("Real time: ");
    Serial1.print((millis() - state.current_timestamp)/1000);
    Serial1.print(" Seconds or ");
    Serial1.print((millis() - state.current_timestamp)/60000UL);
    Serial1.println(" Minutes");
    Serial1.print("Stationary time tracker: ");
    Serial1.print(activity_minutes.stationary);
    Serial1.println(" Minutes.");
    Serial1.println();

  } else if(state.current == walking_state){
    Serial1.println("_____WALKING_____");
    Serial1.print("Current timestamp: ");
    Serial1.println(state.current_timestamp);

    Serial1.print("Real time: ");
    Serial1.print((millis() - state.current_timestamp)/1000);
    Serial1.print(" Seconds or ");
    Serial1.print((millis() - state.current_timestamp)/60000UL);
    Serial1.println(" Minutes");
    Serial1.print("Walking time tracker: ");
    Serial1.print(activity_minutes.walking);
    Serial1.println(" Minutes.");
    Serial1.println();
  } else if(state.current == jogging_state){
    Serial1.println("_____JOGGING_____");
    Serial1.print("Current timestamp: ");
    Serial1.println(state.current_timestamp);

    Serial1.print("Real time: ");
    Serial1.print((millis() - state.current_timestamp)/1000);
    Serial1.print(" Seconds or ");
    Serial1.print((millis() - state.current_timestamp)/60000UL);
    Serial1.println(" Minutes");
    Serial1.print("Jogging time tracker: ");
    Serial1.print(activity_minutes.jogging);
    Serial1.println(" Minutes.");
    Serial1.println();
  } else if(state.current == biking_state){
    Serial1.println("_____BIKING_____");
    Serial1.print("Current timestamp: ");
    Serial1.println(state.current_timestamp);

    Serial1.print("Real time: ");
    Serial1.print((millis() - state.current_timestamp)/1000);
    Serial1.print(" Seconds or ");
    Serial1.print((millis() - state.current_timestamp)/60000UL);
    Serial1.println(" Minutes");
    Serial1.print("Biking time tracker: ");
    Serial1.print(activity_minutes.biking);
    Serial1.println(" Minutes.");
    Serial1.println();
  } else if(state.current == driving_state){
    Serial1.println("_____DRIVING_____");
    Serial1.print("Current timestamp: ");
    Serial1.println(state.current_timestamp);

    Serial1.print("Real time: ");
    Serial1.print((millis() - state.current_timestamp)/1000);
    Serial1.print(" Seconds or ");
    Serial1.print((millis() - state.current_timestamp)/60000UL);
    Serial1.println(" Minutes");
    Serial1.print("Driving time tracker: ");
    Serial1.print(activity_minutes.driving);
    Serial1.println(" Minutes.");
    Serial1.println();
  } else if(state.current == unknown_state){
    Serial1.println("_____UNKNOWN_____");
    Serial1.print("Current timestamp: ");
    Serial1.println(state.current_timestamp);

    Serial1.print("Real time: ");
    Serial1.print((millis() - state.current_timestamp)/1000);
    Serial1.print(" Seconds or ");
    Serial1.print((millis() - state.current_timestamp)/60000UL);
    Serial1.println(" Minutes");
    Serial1.print("Unknown time tracker: ");
    Serial1.print(activity_minutes.unknown);
    Serial1.println(" Minutes.");
    Serial1.println();
  }
}

void update_timers(uint32_t milliseconds){
  float minutes_equivalent = milliseconds/1000/60.0;
  if(state.current == stationary_state){
    activity_minutes.stationary += minutes_equivalent;
  } else if(state.current == walking_state){
    activity_minutes.walking += minutes_equivalent;
  } else if(state.current == jogging_state){
    activity_minutes.jogging += minutes_equivalent;
  } else if(state.current == biking_state){
    activity_minutes.biking += minutes_equivalent;
  } else if(state.current == driving_state){
    activity_minutes.driving += minutes_equivalent;
  } else if(state.current == unknown_state){
    activity_minutes.unknown += minutes_equivalent;
  }
}