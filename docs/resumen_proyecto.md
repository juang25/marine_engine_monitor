# Resumen del Proyecto: Marine Engine Monitor

## Visión General

Sistema de monitorización para motores marinos de embarcaciones de recreo. Un ESP32-WROOM32 (placa KiCad) lee sensores del motor y la sala de máquinas, publica por MQTT a un miniPC con un stack Docker (Mosquitto, Telegraf, InfluxDB, Grafana). Los datos se analizan offline y, a futuro, se prevé detectar anomalías directamente en el ESP32.

---

## Arquitectura de Capas

```
[ESP32 + Sensores]  ──MQTT──>  [MiniPC]
  Firmware Arduino           Mosquitto (broker)
  Lectura 1 Hz                   │
                              Telegraf (MQTT → InfluxDB)
                                  │
                              InfluxDB v2 (bucket: boat)
                                  │
                              Grafana (dashboard + análisis)
```

---

## Hardware

| Componente | Detalle |
|---|---|
| MCU | ESP32-WROOM32, placa KiCad personalizada |
| RPM | GPIO 25, optoacoplador, PCNT periférico HW |
| PT100 (codo escape) | MAX31865 SPI, software SPI (CS=32, MOSI=23, MISO=19, SCK=18), 3-wire |
| T/H sala motor | Sensirion SHT41, I2C (SDA=21, SCL=22) |
| Vbat motor | INA219, I2C (SDA=21, SCL=22) |
| Alarma entrada | IO27, INPUT_PULLUP, activa en GND |
| Alarma salida | IO26, MOSFET, HIGH si alarma activa |
| Alimentación | 12V → R-78E-0.5 (500mA regulator) + condensador 220µF |

---

## Firmware

### Plataforma
- **PlatformIO** con framework Arduino (`espressif32`, `esp32dev`)
- Librerías: PubSubClient, ArduinoJson, Sensirion I2C SHT4x, Adafruit INA219, Adafruit MAX31865

### Módulos

| Archivo | Responsabilidad |
|---|---|
| `main.cpp` | Loop principal (WiFi → RPM → MQTT → sensores → publish) |
| `sensors.cpp` | Inicialización y lectura de todos los sensores |
| `wifi_manager.cpp` | Conexión WiFi (SSID: ASUS), reconexión automática, TX power 8.5dBm |
| `mqtt_manager.cpp` | Cliente MQTT a `192.168.1.235:1883`, publicación JSON cada 1s |
| `rpmsensor.cpp` | PCNT HW en GPIO 25, polling cada 500ms, sin ISR |
| `sensor_data.h` | Struct `EngineData` con todos los campos |

### Flujo del Loop Principal

```
loop()
  ├── handleWiFi()        → reconexión si es necesario
  ├── rpmSensor.poll()    → lee contador PCNT cada 500ms
  ├── handleMQTT()        → client.loop() + reconexión MQTT
  ├── mqttLoop()          → client.loop() adicional
  ├── handleSensors()     → actualiza EngineData (MAX31865, SHT41, INA219, alarmas)
  └── handlePublish()     → JSON {rpm, exhaust_temp, battery_voltage, ...}
```

### Payload MQTT

```json
{
  "rpm": 550,
  "exhaust_temp": 28.98,
  "battery_voltage": 0.86,
  "engine_room_temp": 27.11,
  "engine_room_humidity": 54.04,
  "alarm_input_active": false
}
```
- Topic: `boat/engine/data`
- QoS: 0
- Frecuencia: ~1 Hz

### RPM Sensor (PCNT)

El sensor de RPM usa el periférico **PCNT** (Pulse Counter) del ESP32. No requiere interrupciones ni CPU — cuenta pulsos en hardware.

- GPIO 25, filtro HW de 100 ciclos APB (~1.25 µs)
- `poll()` se llama desde `loop()` cada ≥500 ms
- Lee contador, lo limpia, calcula Hz: `count × 1000 / dt`
- `getHz()` devuelve la última frecuencia medida

---

## Backend (MiniPC)

### Stack Docker

| Servicio | Imagen | Puerto | Función |
|---|---|---|---|
| Mosquitto | eclipse-mosquitto:2 | 1883 (TCP) | Broker MQTT |
| InfluxDB | influxdb:2.7 | 8086 | Almacenamiento time-series |
| Telegraf | telegraf:latest | — | Suscriptor MQTT → escritor InfluxDB |
| Grafana | grafana/grafana:latest | 3000 | Visualización y dashboards |

### Configuración InfluxDB v2
- Organization: `marine`
- Bucket: `boat`
- Token: `mytoken123`

### Telegraf
- Subscribe `boat/engine/data` desde `host.docker.internal:1883`
- `data_format = "json"`, `name_override = "engine"`
- Output a InfluxDB v2 (`http://influxdb:8086`)

### Grafana Provisioning
- Datasource InfluxDB auto-configurado (UID: `influxdb-marine`)
- Dashboard `Marine Engine Monitor` con:
  - Fila 1: Gauges (RPM, Exhaust Temp, Battery, Engine Room Temp, Humidity)
  - Fila 2: Time series RPM (últ. 30 min)
  - Fila 3: Time series Exhaust Temp (últ. 30 min)

---

## Problemas y Soluciones

### 1. Brownout (crash por alimentación) — recurrente
- **Síntoma:** `Brownout detector triggered` tras minutos/horas de funcionamiento
- **Causa:** regulador R-78E-0.5 (500mA) insuficiente para picos de WiFi. Condensador de 10µF no filtraba transitorios
- **Soluciones:**
  - Reducción WiFi TX power: 20dBm → **8.5dBm** (RSSI suficiente: -36 dBm)
  - Retry MQTT espaciado a **10s** para evitar picos consecutivos
  - Añadir condensador electrolítico **220µF** en paralelo con el de 10µF cerca del ESP32
  - Re-soldadura del optoacoplador (problema de hardware inicial)

### 2. PT100 leía -242°C
- **Causa:** MAX31865 configurado como 2-wire, CS pin compartido con otro periférico
- **Solución:** `MAX31865_3WIRE`, software SPI explícito, CS dedicado en IO32

### 3. RPM — glitch con señal seno bipolar
- **Síntoma:** Lecturas erráticas (hasta 800 pulsos/s) con señal seno ±10V
- **Causa:** Zero-crossing lento de la seno causaba rebotes en el optoacoplador
- **Conclusión:** No es problema real — el alternador del barco genera señal **unipolar** (0-Vbat) con flancos rápidos

### 4. RPM — Interferencia WiFi/MQTT con GPIO ISR
- **Causa:** ISR en GPIO 25 para contar pulsos; WiFi/MQTT interrumpían el conteo
- **Solución:** Se reemplazó GPIO ISR por **PCNT periférico HW** que cuenta independientemente de la CPU

### 5. LoadProhibited Crash en Startup
- **Síntoma:** `Guru Meditation Error (LoadProhibited)` justo al activar WiFi
- **Causa:** Timer ISR del RPM intentaba ejecutar `pcnt_get_counter_value()` (en flash) mientras WiFi accedía al flash
- **Solución:** Se eliminó el timer ISR por completo. PCNT se lee por **polling desde `loop()`** — sin ISR, sin conflicto de flash

### 6. MQTT no conectaba o tardaba minutos
- **Síntoma:** `errno 113 (Software caused connection abort)` repetido durante 2-3 minutos
- **Causa 1:** El TCP connect interno de PubSubClient usaba timeout lwIP por defecto (~21s con retransmisiones SYN)
- **Causa 2:** Conflicto de VPN en el miniPC que abortaba conexiones TCP entrantes
- **Soluciones:**
  - `espClient.setTimeout(3000)` — timeout de socket 3s
  - `client.setKeepAlive(30)` — reduce tráfico PINGREQ
  - Log rate-limited a 10s para evitar spam
  - El conflicto VPN se resolvió manualmente

### 7. ESP32 se colgaba tras minutos publicando
- **Síntoma:** El ESP dejaba de enviar datos, sin crash aparente
- **Causa:** Fragmentación de heap por `JsonDocument` creado en cada `publishData()` + socket TCP muerto no detectado
- **Soluciones:**
  - `JsonDocument` y buffer **estáticos** (asignación única)
  - `client.loop()` llamado **antes** de `client.connected()` en `handleMQTT`

---

## Modelo Térmico Planeado (no implementado)

### Ecuación diferencial de 1er orden

```
dT/dt = (T_steady(RPM) − T) / τ
```

### Parámetros por bin de 100 RPM (0-3000)

- `T_steady[bin]` — temperatura final del codo a esas RPM (EMA α=0.001)
- `τ_heat[bin]` — inercia térmica al calentar (EMA α=0.01)
- `τ_cool[bin]` — inercia al enfriar (EMA α=0.01)
- `σ²[bin]` — varianza del residuo (EMA α=0.05)

### Máquina de Estados

```
OFF → STARTING → WARMING → STEADY ↔ COOLING → STOPPING → OFF
```

### Detección de Anomalías

- Cada segundo: `T_pred = T + dt/τ × (T_steady[bin] − T)`
- Si `|T − T_pred| > 3σ` → WARN; `> 6σ` → ALARM
- Aprendizaje online no supervisado — sin datos etiquetados
- Adaptación estacional automática por EMA lenta

---

## Recomendaciones Pendientes

| Tema | Acción |
|---|---|
| PULSES_PER_REV | Calibrar en el motor real con tacómetro |
| Tª agua de mar | Añadir sensor DS18B20 para mejorar modelo |
| Portabilidad redes | Implementar mDNS o WiFiManager para distintas subredes |
| Modelo térmico | Implementar en ESP32 tras validar con datos reales |
| Anomalías de batería | Añadir detección de caída OFF, mínima arranque, carga alternador |

---

## Mapa de Archivos Clave

```
firmware/
├── platformio.ini                          # Dependencias y config build
├── include/
│   ├── RPMSensor.h                         # Clase RPM (PCNT)
│   ├── mqtt_manager.h                      # Setup/handle/publish MQTT
│   ├── sensor_data.h                       # Struct EngineData
│   ├── sensors.h                           # Init/read sensores
│   └── wifi_manager.h                     # Setup/handle WiFi
└── src/
    ├── main.cpp                            # Punto de entrada
    ├── mqtt_manager.cpp                    # Cliente MQTT + JSON publish
    ├── rpmsensor.cpp                       # PCNT + polling
    ├── sensors.cpp                         # Lectura sensores I2C/SPI
    └── wifi_manager.cpp                    # Conexión WiFi

backend/
├── docker-compose.yaml                     # Stack Docker (Mosquitto, InfluxDB, Telegraf, Grafana)
├── telegraf/telegraf.conf                  # MQTT → InfluxDB
└── grafana/provisioning/
    ├── datasources/datasource.yml          # Data source InfluxDB
    └── dashboards/
        ├── dashboard.yml                   # Dashboard provider
        └── marine-engine-monitor.json      # Dashboard JSON

docs/
└── resumen_proyecto.md                     # Este documento
```
