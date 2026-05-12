#include "mqtt_manager.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "sensor_data.h"
#include "sensor_simulator.h"

WiFiClient espClient;
PubSubClient client(espClient);

const char *mqtt_server = "192.168.1.235";

void setupMQTT()
{
    client.setServer(mqtt_server, 1883);
}

void reconnectMQTT()
{
    while (!client.connected())
    {
        Serial.print("Conectando MQTT...");

        if (client.connect("ESP32Client"))
        {
            Serial.println("conectado");
        }
        else
        {
            Serial.print("fallo, rc=");
            Serial.print(client.state());
            Serial.println(" reintentando en 5 segundos");

            delay(5000);
        }
    }
}

void publishTestMessage()
{
    EngineData data;
    updateSimulation(data);
    JsonDocument doc;

    doc["rpm"] = data.rpm;
    doc["coolant_temp"] = data.coolantTemp;

    doc["battery_voltage"] = data.batteryVoltage;
    serializeJson(doc, Serial);
    Serial.println();
    char buffer[256];

    serializeJson(doc, buffer);

    client.publish("boat/engine/data", buffer);
}

void mqttLoop()
{
    client.loop();
}