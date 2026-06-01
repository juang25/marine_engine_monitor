#include "mqtt_manager.h"
#include "wifi_manager.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

WiFiClient espClient;
PubSubClient client(espClient);

const char *mqttServer = "192.168.1.235";

static bool reconnectMQTT();
static void publishData(const EngineData &data);

void setupMQTT()
{
    client.setServer(mqttServer, 1883);
    client.setKeepAlive(30);
    espClient.setTimeout(3000);
    Serial.print("[MQTT] Server=");
    Serial.println(mqttServer);
}

void handleMQTT(unsigned long now)
{
    static unsigned long lastAttempt = 0;
    static bool waitingForWiFiLogged = false;

    client.loop();

    if (client.connected())
    {
        waitingForWiFiLogged = false;
        return;
    }

    if (!WiFi.isConnected())
    {
        if (!waitingForWiFiLogged)
        {
            Serial.println("[MQTT] Waiting for WiFi");
            waitingForWiFiLogged = true;
        }

        return;
    }

    waitingForWiFiLogged = false;

    unsigned long wifiSince = wifiConnectedSinceMs();
    if (wifiSince > 0 && now - wifiSince < 2000)
    {
        return;
    }

    if (now - lastAttempt < 10000)
    {
        return;
    }

    lastAttempt = now;

    reconnectMQTT();
}

void handlePublish(
    unsigned long now,
    const EngineData &data)
{
    static unsigned long lastPublish = 0;

    if (!client.connected())
    {
        return;
    }

    if (now - lastPublish < 1000)
    {
        return;
    }

    lastPublish = now;

    publishData(data);
}

void mqttLoop()
{
    client.loop();
}

static bool reconnectMQTT()
{
    static unsigned long lastLog = 0;
    unsigned long now = millis();

    char clientId[24];
    snprintf(clientId, sizeof(clientId), "mem-%llx", ESP.getEfuseMac());

    if (client.connect(clientId, NULL, NULL, 0, 0, 0, 0, true))
    {
        Serial.println("[MQTT] Connected");
        return true;
    }

    if (now - lastLog > 10000)
    {
        lastLog = now;
        Serial.print("[MQTT] FAILED, rc=");
        Serial.print(client.state());
        Serial.println();
    }

    return false;
}

static void publishData(const EngineData &data)
{
    static JsonDocument doc;
    static char buffer[256];

    doc["rpm"] = data.rpm;

    doc["exhaust_temp"] =
        data.exhaustTemp;

    doc["battery_voltage"] =
        data.batteryVoltage;

    doc["engine_room_temp"] =
        data.engineRoomTemp;

    doc["engine_room_humidity"] =
        data.engineRoomHumidity;

    doc["alarm_input_active"] =
        data.alarmInputActive;

    serializeJson(doc, buffer);

    doc.clear();

    client.publish(
        "boat/engine/data",
        buffer);

    Serial.print("[MQTT] Published: ");

    Serial.print(buffer);

    Serial.println();
}
