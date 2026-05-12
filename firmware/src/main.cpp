#include <Arduino.h>
#include <WiFi.h>

#include "wifi_manager.h"
#include "mqtt_manager.h"

unsigned long lastMsg = 0;

void setup()
{
    Serial.begin(115200);

    setupWiFi();
    delay(3000);

    WiFiClient testClient;


    setupMQTT();
}
void loop()
{
    if (!WiFi.isConnected())
    {
        Serial.println("WiFi desconectado");
    }

    if (!client.connected())
    {
        reconnectMQTT();
    }

    mqttLoop();

    unsigned long now = millis();

    if (now - lastMsg > 1000)
    {
        lastMsg = now;

        publishTestMessage();

        Serial.println("Mensaje MQTT enviado");
    }
}