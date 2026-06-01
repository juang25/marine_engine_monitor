#pragma once

#include <SensirionI2cSht4x.h>
#include "sensor_data.h"
#include <SPI.h>
#include <Adafruit_MAX31865.h>

extern SensirionI2cSht4x sht4x;
void initSensors();

void updateSHT41(EngineData &data);
void handleSensors(
    unsigned long now,
    EngineData &data);
void scanI2C();
