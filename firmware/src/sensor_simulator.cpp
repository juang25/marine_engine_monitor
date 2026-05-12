#include "sensor_simulator.h"
#include "sensor_data.h"
float rpm = 800;
bool increasing = true;

void updateSimulation(EngineData &data)
{
    if (increasing)
    {
        rpm += 100;

        if (rpm > 3000)
        {
            increasing = false;
        }
    }
    else
    {
        rpm -= 100;

        if (rpm < 800)
        {
            increasing = true;
        }
    }
    data.rpm = rpm;
    data.coolantTemp = 60 + (rpm / 100.0);
    data.batteryVoltage = 13.8 - (rpm * 0.0001);
}