#include <Arduino.h>
#include <WiFi.h>

#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "Wire.h"
#include "SensirionI2cSht4x.h"
#include "sensor_data.h"
#include "sensors.h"
#include "RPMSensor.h"

unsigned long lastMsg = 0;

EngineData data;

RPMSensor rpmSensor;

void setup()
{
    Serial.begin(115200);

    Serial.println();
    Serial.println("Marine Engine Monitor starting...");

    initSensors();

    setupWiFi();

    // scanI2C();

    setupMQTT();

    Serial.println("System initialized");
}

unsigned long lastSensorRead = 0;
unsigned long lastPublish = 0;

void loop()
{
    unsigned long now = millis();
    handleWiFi(now);
    rpmSensor.poll();

    handleMQTT(now);
    mqttLoop();
    now = millis();
    handleSensors(now, data);

    handlePublish(now, data);
}