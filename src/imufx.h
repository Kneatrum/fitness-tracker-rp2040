#ifndef IMUFX_H
#define IMUFX_H

#include "LSM6DSOXSensor.h"
#include "lsm6dsox_activity_recognition_for_mobile.h"

#define DEV_I2C Wire

#define INT_1 INT_IMU

#define ACTIVITY_REPORTING_PERIOD_MS     5000
#define STEPS_REPORTING_PERIOD_MS  5000
extern uint32_t oneSecondRefreshTime;
extern uint32_t activityDataSampleTime;
extern uint32_t stepsDataSampleTime;
extern LSM6DSOXSensor AccGyr;

extern int32_t LineCounter;
extern int32_t TotalNumberOfLine;

//Interrupts.
extern volatile int mems_event;
extern uint16_t step_count;

extern unsigned long INVALID_TIMESTAMP;
extern uint8_t INITIALIZING_STATE;
extern uint8_t stationary_state;
extern uint8_t walking_state;
extern uint8_t jogging_state;
extern uint8_t biking_state;
extern uint8_t driving_state;
extern uint8_t sleeping_state;
extern uint8_t unknown_state;

extern const char* activity_names[14];


struct MinutesTracker {
    double stationary;
    double walking;
    double jogging;
    double biking;
    double driving;
    double sleeping;
    double unknown;
};

extern MinutesTracker activity_minutes;
MinutesTracker InitializeMinutesTracker();

struct sleepTime {
    uint32_t awakeMinutes;
    uint32_t lightSleepMinutes;
    uint32_t remSleepMinutes;
    uint32_t deepSleepMinutes;
};

struct Switch{
    uint8_t current;
    unsigned long current_timestamp;
    uint8_t previous;
    unsigned long previous_timestamp;
    double prev_state_time_taken;
};

extern Switch state;

Switch InitializeSwitch();


void getActivitySummary(uint8_t state);

void calculatePrevStateTime();

void display_progress();

void update_timers(uint32_t milliseconds);

bool load_mlc_program();

void printMLCStatus(uint8_t status);

void imu_mlc_init();

void INT1Event_cb();

#endif