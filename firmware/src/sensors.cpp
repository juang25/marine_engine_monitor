#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "sensor_data.h"
#include "sensors.h"
#include <SensirionI2cSht4x.h>
#include <Adafruit_INA219.h>
#include "rpmsensor.h"
#include "log_buffer.h"

Adafruit_INA219 ina219;
extern RPMSensor rpmSensor;

SensirionI2cSht4x sht4x;
// Pines SPI (VSPI)
#define MAX_CS 32
#define MAX_MOSI 23
#define MAX_MISO 19
#define MAX_SCK 18
#define ALARM_INPUT_PIN 27
#define ALARM_OUTPUT_PIN 26

// Software SPI: CS, DI/MOSI, DO/MISO, CLK. Keeps the wiring explicit on bare ESP32-WROOM boards.
Adafruit_MAX31865 thermo = Adafruit_MAX31865(
    MAX_CS, MAX_MOSI, MAX_MISO, MAX_SCK);
// The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
#define RREF 430.0
// The 'nominal' 0-degrees-C resistance of the sensor
// 100.0 for PT100, 1000.0 for PT1000
#define RNOMINAL 100.0

static void printMAX31865Fault(uint8_t fault);

void initSensors()
{
    Wire.begin(21, 22);

    sht4x.begin(Wire, 0x44);
    if (!ina219.begin())
    {
        Serial.println("[INA219] NOT FOUND");
    }
    else
    {
        Serial.println("[INA219] OK");
    }
    Serial.println("Adafruit MAX31865 PT100 Sensor Test!");

    thermo.begin(MAX31865_3WIRE);
    Serial.println("[PT100] MAX31865 initialized, checking faults on read");
    pinMode(ALARM_INPUT_PIN, INPUT_PULLUP);
    pinMode(ALARM_OUTPUT_PIN, OUTPUT);
    digitalWrite(ALARM_OUTPUT_PIN, LOW);
    rpmSensor.begin();
}

void updateSHT41(EngineData &data)
{
    float temperature;
    float humidity;

    int error = sht4x.measureHighPrecision(
        temperature,
        humidity);

    if (!error)
    {
        data.engineRoomTemp = temperature;
        data.engineRoomHumidity = humidity;
    }
}
void updateINA219(EngineData &data)
{
    data.batteryVoltage = ina219.getBusVoltage_V();
}

void updateAlarmInput(EngineData &data)
{
    data.alarmInputActive = digitalRead(ALARM_INPUT_PIN) == LOW;
    digitalWrite(ALARM_OUTPUT_PIN, data.alarmInputActive ? HIGH : LOW);
}

void updateMAX31865(EngineData &data)
{
    uint16_t rtd = thermo.readRTD();
    float resistance = (rtd * RREF) / 32768.0;
    float temperature = thermo.temperature(RNOMINAL, RREF);
    uint8_t fault = thermo.readFault();

    /* Serial.print("[PT100] RTD raw: ");
     Serial.print(rtd);
     Serial.print("  Resistance: ");
     Serial.print(resistance);
     Serial.print(" Ohms, Temperature: ");
     Serial.print(temperature);
     Serial.println(" °C");
     */

    if (fault)
    {
        printMAX31865Fault(fault);
        thermo.clearFault();
        return;
    }

    if (rtd == 0 || temperature < -200.0)
    {
        Serial.println("[PT100] Invalid reading: RTD near zero. Check PT100 wiring, MAX31865 wire mode and SPI pins.");
        return;
    }

    data.exhaustTemp = temperature;
}

static void printMAX31865Fault(uint8_t fault)
{
    Serial.print("[PT100] Fault 0x");
    Serial.println(fault, HEX);

    if (fault & MAX31865_FAULT_HIGHTHRESH)
        Serial.println("[PT100] RTD high threshold");
    if (fault & MAX31865_FAULT_LOWTHRESH)
        Serial.println("[PT100] RTD low threshold");
    if (fault & MAX31865_FAULT_REFINLOW)
        Serial.println("[PT100] REFIN- > 0.85 x bias, check wiring");
    if (fault & MAX31865_FAULT_REFINHIGH)
        Serial.println("[PT100] REFIN- < 0.85 x bias, check wiring");
    if (fault & MAX31865_FAULT_RTDINLOW)
        Serial.println("[PT100] RTDIN- < 0.85 x bias, check wiring");
    if (fault & MAX31865_FAULT_OVUV)
        Serial.println("[PT100] Over/under voltage fault");
}

void updateSensors(EngineData &data)
{
    updateSHT41(data);
    updateINA219(data);
    updateMAX31865(data);
    updateAlarmInput(data);
    data.rpm = rpmSensor.getHz();
}

void handleSensors(
    unsigned long now,
    EngineData &data)
{
    static unsigned long lastRead = 0;

    if (now - lastRead < 1000)
    {
        return;
    }

    lastRead = now;

    updateSensors(data);

    logPrintf(
        "T codo: %.2f \xc2\xba"
        "C  Temp: %.2f \xc2\xba"
        "C  Hum: %.2f  V Bat: %.2f V RPM: %.2f rpm  Alarma: %s\n",
        data.exhaustTemp,
        data.engineRoomTemp,
        data.engineRoomHumidity,
        data.batteryVoltage,
        data.rpm,
        data.alarmInputActive ? "ON" : "OFF");
}
