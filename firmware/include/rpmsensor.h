#pragma once
#include <Arduino.h>

class RPMSensor
{
public:
    RPMSensor();
    void begin();
    float getHz();
    void poll();

private:
    volatile float latestRPM;
    unsigned long lastPoll;
    void setupPCNT();
};
