#include "imufx.h"

LSM6DSOXSensor AccGyr(&DEV_I2C, LSM6DSOX_I2C_ADD_L);

int32_t LineCounter;
int32_t TotalNumberOfLine;

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



void printMLCStatus(uint8_t status) {
  switch(status) {
    case 0:
      Serial1.println("Activity: Stationary");
    //   mqtt_send_str(test, "Stationary");
      break;
    case 1:
      Serial1.println("Activity: Walking");
    //   mqtt_send_str(test, "Walking");
      break;
    case 4:
      Serial1.println("Activity: Jogging");
    //   mqtt_send_str(test, "Jogging");
      break;
    case 8:
      Serial1.println("Activity: Biking");
    //   mqtt_send_str(test, "Biking");
      break;
    case 12:
      Serial1.println("Activity: Driving");
    //   mqtt_send_str(test, "Driving");
      break;
    default:
      Serial1.println("Activity: Unknown");
    //   mqtt_send_str(test, "Unknown");
      break;
  }    
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
    printMLCStatus(mlc_out[0]);

}