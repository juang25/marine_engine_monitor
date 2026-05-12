#include "wifi_manager.h"
#include <WiFi.h>

const char *ssid = "ASUS";
const char *password = "IX1V7602698AD494935";

void setupWiFi()
{
    Serial.println();
    Serial.print("Conectando a WiFi");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi conectado");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}