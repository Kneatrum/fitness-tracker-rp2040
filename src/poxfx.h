#ifndef POXFX_H
#define POXFX_H

#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS     1000

extern PulseOximeter pox;

extern uint32_t tsLastReport;
extern bool time_to_print;

void onBeatDetected();
bool pox_init();


#endif