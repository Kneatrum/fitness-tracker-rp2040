#ifndef IMUFX_H
#define IMUFX_H

#include "LSM6DSOXSensor.h"
#include "lsm6dsox_activity_recognition_for_mobile.h"

#define DEV_I2C Wire

#define INT_1 INT_IMU

extern LSM6DSOXSensor AccGyr;

extern int32_t LineCounter;
extern int32_t TotalNumberOfLine;

//Interrupts.
extern volatile int mems_event;
extern uint16_t step_count;

bool load_mlc_program();

void printMLCStatus(uint8_t status);

void imu_mlc_init();

void INT1Event_cb();

#endif