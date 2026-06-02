#include "wifi_manager.h"
#include "log_buffer.h"
#include <WiFi.h>

typedef struct
{
    const char *ssid;
    const char *password;
} WifiNet;

static const WifiNet networks[] = {
    {"ASUS", "IX1V7602698AD494935"},
    {"pamara_barco_2", "FC51679400"},
};

static const int numNetworks = sizeof(networks) / sizeof(networks[0]);
static int currentNet = 0;

static void connectWiFi();
static void nextWiFi();
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
            logPrintf("[WiFi] Connected, IP=%s\r\n", WiFi.localIP().toString().c_str());
            logPrintf("[WiFi] Gateway=%s\r\n", WiFi.gatewayIP().toString().c_str());
            logPrintf("[WiFi] Subnet=%s\r\n", WiFi.subnetMask().toString().c_str());
            logPrintf("[WiFi] RSSI=%d dBm\r\n", WiFi.RSSI());
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

    logPrintf("[WiFi] Disconnected, status=%s\r\n", wifiStatusName(WiFi.status()));

    nextWiFi();
}

static void connectWiFi()
{
    Serial.print("[WiFi] Trying net ");
    Serial.print(currentNet);
    Serial.print(": ");
    Serial.println(networks[currentNet].ssid);

    WiFi.begin(networks[currentNet].ssid, networks[currentNet].password);
}

static void nextWiFi()
{
    if (WiFi.isConnected())
        return;

    currentNet = (currentNet + 1) % numNetworks;
    connectWiFi();
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
