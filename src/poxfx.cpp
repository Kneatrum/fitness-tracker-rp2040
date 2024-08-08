#include "poxfx.h"


PulseOximeter pox;

uint32_t tsLastReport = 0;
bool time_to_print = false;

// Callback (registered below) fired when a pulse is detected
void onBeatDetected()
{
    // Serial1.println("Beat!");
    time_to_print = true;
}


bool pox_init(){

    Serial1.println("Initializing pulse oximeter");
    if(!pox.begin()) {
        Serial1.print("FAILED");
        return false;
    } 

    Serial1.println("SUCCESS");
    pox.setOnBeatDetectedCallback(onBeatDetected);
    return true;
}




