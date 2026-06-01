#include "wifi_manager.h"
#include <WiFi.h>

const char *ssid = "ASUS";
const char *password = "IX1V7602698AD494935";

static void connectWiFi();
static const char *wifiStatusName(wl_status_t status);
unsigned long wifiConnectedSince = 0;

void setupWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);

    connectWiFi();
}

void handleWiFi(unsigned long now)
{
    static unsigned long lastAttempt = 0;
    static bool wasConnected = false;

    if (WiFi.isConnected())
    {
        if (!wasConnected)
        {
            wifiConnectedSince = now;
            Serial.print("[WiFi] Connected, IP=");
            Serial.println(WiFi.localIP());
            Serial.print("[WiFi] Gateway=");
            Serial.println(WiFi.gatewayIP());
            Serial.print("[WiFi] Subnet=");
            Serial.println(WiFi.subnetMask());
            Serial.print("[WiFi] RSSI=");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm");
            wasConnected = true;
        }

        return;
    }

    wasConnected = false;

    if (now - lastAttempt < 10000)
    {
        return;
    }

    lastAttempt = now;

    Serial.print("[WiFi] Disconnected, status=");
    Serial.println(wifiStatusName(WiFi.status()));

    connectWiFi();
}

static void connectWiFi()
{
    Serial.print("[WiFi] Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
}

unsigned long wifiConnectedSinceMs()
{
    return wifiConnectedSince;
}

static const char *wifiStatusName(wl_status_t status)
{
    switch (status)
    {
    case WL_IDLE_STATUS:
        return "IDLE";
    case WL_NO_SSID_AVAIL:
        return "NO_SSID";
    case WL_SCAN_COMPLETED:
        return "SCAN_COMPLETED";
    case WL_CONNECTED:
        return "CONNECTED";
    case WL_CONNECT_FAILED:
        return "CONNECT_FAILED";
    case WL_CONNECTION_LOST:
        return "CONNECTION_LOST";
    case WL_DISCONNECTED:
        return "DISCONNECTED";
    default:
        return "UNKNOWN";
    }
}
