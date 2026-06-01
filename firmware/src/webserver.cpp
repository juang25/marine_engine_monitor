#include <Arduino.h>
#include <WiFi.h>
#include "webserver.h"
#include "sensor_data.h"

extern EngineData data;

WiFiServer httpServer(80);

static void handleClient(WiFiClient &client);

void setupWebServer()
{
    httpServer.begin();
    Serial.print("[HTTP] Server started on http://");
    Serial.print(WiFi.localIP());
    Serial.println(":80");
}

void handleWebServer()
{
    WiFiClient client = httpServer.available();
    if (client)
        handleClient(client);
}

static void handleClient(WiFiClient &client)
{
    if (!client.connected())
        return;

    String request = client.readStringUntil('\n');
    while (client.available() && !client.find("\r\n\r\n")) {}

    bool isApi = request.indexOf("GET /api") >= 0;

    if (isApi)
    {
        String body = "{";
        body += "\"rpm\":" + String(data.rpm, 1) + ",";
        body += "\"exhaust_temp\":" + String(data.exhaustTemp, 2) + ",";
        body += "\"engine_room_temp\":" + String(data.engineRoomTemp, 2) + ",";
        body += "\"engine_room_humidity\":" + String(data.engineRoomHumidity, 2) + ",";
        body += "\"battery_voltage\":" + String(data.batteryVoltage, 3);
        body += "}";
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.print("Content-Length: ");
        client.println(body.length());
        client.println();
        client.println(body);
    }
    else
    {
        String body = "<!DOCTYPE html><html><head>";
        body += "<meta charset='UTF-8'>";
        body += "<meta http-equiv='refresh' content='2'>";
        body += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
        body += "<title>Marine Engine Monitor</title>";
        body += "<style>";
        body += "body{font-family:Arial,sans-serif;margin:20px;background:#1a1a2e;color:#eee;text-align:center}";
        body += "h1{color:#e94560}";
        body += ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:16px;max-width:800px;margin:auto}";
        body += ".card{background:#16213e;border-radius:12px;padding:20px;box-shadow:0 4px 8px rgba(0,0,0,0.3)}";
        body += ".label{font-size:14px;color:#aaa;margin-bottom:8px}";
        body += ".value{font-size:28px;font-weight:bold}";
        body += ".rpm{color:#e94560}";
        body += ".temp{color:#f5a623}";
        body += ".volt{color:#4ade80}";
        body += ".hum{color:#60a5fa}";
        body += "</style></head><body>";
        body += "<h1>Marine Engine Monitor</h1>";
        body += "<div class='grid'>";

        body += "<div class='card'><div class='label'>RPM</div>";
        body += "<div class='value rpm'>" + String(data.rpm, 1) + "</div></div>";

        body += "<div class='card'><div class='label'>Exhaust Temp</div>";
        body += "<div class='value temp'>" + String(data.exhaustTemp, 1) + " &deg;C</div></div>";

        body += "<div class='card'><div class='label'>Engine Room Temp</div>";
        body += "<div class='value temp'>" + String(data.engineRoomTemp, 1) + " &deg;C</div></div>";

        body += "<div class='card'><div class='label'>Humidity</div>";
        body += "<div class='value hum'>" + String(data.engineRoomHumidity, 1) + " %</div></div>";

        body += "<div class='card'><div class='label'>Battery</div>";
        body += "<div class='value volt'>" + String(data.batteryVoltage, 2) + " V</div></div>";

        body += "</div></body></html>";

        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");
        client.print("Content-Length: ");
        client.println(body.length());
        client.println();
        client.println(body);
    }

    client.stop();
}
