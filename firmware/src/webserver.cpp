#include <Arduino.h>
#include <WiFi.h>
#include "webserver.h"
#include "sensor_data.h"
#include "log_buffer.h"

extern EngineData data;

WiFiServer httpServer(80);

static void handleClient(WiFiClient &client);
static String buildHtml();
static String buildJson();

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

    unsigned long t0 = millis();
    String firstLine;

    while (millis() - t0 < 200)
    {
        if (client.available())
        {
            char c = client.read();
            if (c == '\n')
                break;
            firstLine += c;
        }
        yield();
    }

    while (client.available())
        client.read();

    bool isApi = firstLine.startsWith("GET /api");

    String body = isApi ? buildJson() : buildHtml();
    const char *contentType = isApi ? "application/json" : "text/html";

    client.println("HTTP/1.1 200 OK");
    client.print("Content-Type: ");
    client.println(contentType);
    client.println("Connection: close");
    client.println("Cache-Control: no-cache, no-store, must-revalidate");
    client.println("Pragma: no-cache");
    client.println("Expires: 0");
    client.print("Content-Length: ");
    client.println(body.length());
    client.println();
    client.print(body);
    client.flush();
    client.stop();
}

static String buildHtml()
{
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta http-equiv='refresh' content='2'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>Marine Engine Monitor</title>";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;margin:20px;background:#1a1a2e;color:#eee;text-align:center}";
    html += "h1{color:#e94560}";
    html += ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:16px;max-width:800px;margin:auto}";
    html += ".card{background:#16213e;border-radius:12px;padding:20px;box-shadow:0 4px 8px rgba(0,0,0,0.3)}";
    html += ".label{font-size:14px;color:#aaa;margin-bottom:8px}";
    html += ".value{font-size:28px;font-weight:bold}";
    html += ".rpm{color:#e94560}";
    html += ".temp{color:#f5a623}";
    html += ".volt{color:#4ade80}";
    html += ".hum{color:#60a5fa}";
    html += ".log-container{max-width:800px;margin:30px auto 0;text-align:left}";
    html += ".log-container h2{color:#e94560;font-size:18px;margin-bottom:8px}";
    html += ".log{background:#0f172a;color:#e2e8f0;font-family:'Consolas','Courier New',monospace;font-size:13px;padding:12px;border-radius:8px;max-height:300px;overflow-y:auto;white-space:pre-wrap;word-break:break-all;border:1px solid #334155;margin:0}";
    html += "</style></head><body>";
    html += "<h1>Marine Engine Monitor</h1>";
    html += "<div class='grid'>";

    html += "<div class='card'><div class='label'>RPM</div>";
    html += "<div class='value rpm'>" + String(data.rpm, 1) + "</div></div>";

    html += "<div class='card'><div class='label'>Exhaust Temp</div>";
    html += "<div class='value temp'>" + String(data.exhaustTemp, 1) + " &deg;C</div></div>";

    html += "<div class='card'><div class='label'>Engine Room Temp</div>";
    html += "<div class='value temp'>" + String(data.engineRoomTemp, 1) + " &deg;C</div></div>";

    html += "<div class='card'><div class='label'>Humidity</div>";
    html += "<div class='value hum'>" + String(data.engineRoomHumidity, 1) + " %</div></div>";

    html += "<div class='card'><div class='label'>Battery</div>";
    html += "<div class='value volt'>" + String(data.batteryVoltage, 2) + " V</div></div>";

    html += "</div>";
    html += "<div class='log-container'><h2>Console</h2>";
    html += "<pre class='log'>";
    int n = getLogCount();
    int start = n > LOG_LINES ? n - LOG_LINES : 0;
    for (int i = start; i < n; i++)
    {
        String line = getLogLine(i);
        line.replace("&", "&amp;");
        line.replace("<", "&lt;");
        line.replace(">", "&gt;");
        html += line;
    }
    html += "</pre></div>";
    html += "<script>var l=document.querySelector('.log');if(l)l.scrollTop=l.scrollHeight;</script>";
    html += "</body></html>";
    return html;
}

static String buildJson()
{
    String json = "{";
    json += "\"rpm\":" + String(data.rpm, 1) + ",";
    json += "\"exhaust_temp\":" + String(data.exhaustTemp, 2) + ",";
    json += "\"engine_room_temp\":" + String(data.engineRoomTemp, 2) + ",";
    json += "\"engine_room_humidity\":" + String(data.engineRoomHumidity, 2) + ",";
    json += "\"battery_voltage\":" + String(data.batteryVoltage, 3);
    json += "}";
    return json;
}
