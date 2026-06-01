#include <Arduino.h>
#include <Wire.h>

void scanI2C()
{
    byte error;
    byte address;

    Serial.println("Scanning I2C...");

    for (address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);

        error = Wire.endTransmission();

        if (error == 0)
        {
            Serial.print("I2C device found at 0x");

            if (address < 16)
            {
                Serial.print("0");
            }

            Serial.println(address, HEX);
        }
    }

    Serial.println("Scan done");
}