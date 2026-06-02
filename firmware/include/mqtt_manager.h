#pragma once

#include "sensor_data.h"

void setupMQTT();

void handleMQTT(unsigned long now);

void handlePublish(
    unsigned long now,
    const EngineData& data
);

void mqttLoop();
