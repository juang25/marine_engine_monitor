#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

struct EngineData
{
    float rpm;
    float exhaustTemp;
    float batteryVoltage;
    float engineRoomTemp;
    float engineRoomHumidity;
    bool alarmInputActive;
};

#endif
