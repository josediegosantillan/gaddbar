# 📋 Documentación Completa del Proyecto: Gadd Bar IoT

> **Propósito**: Este documento contiene toda la información necesaria para que una IA o desarrollador pueda replicar el proyecto desde cero.

---

## 📁 Estructura del Proyecto

```
gaddbar/
├── platformio.ini          # Configuración de PlatformIO
├── include/
│   └── README
├── lib/
│   └── README
├── src/
│   ├── config.h            # Configuración global (credenciales, pines, tópicos)
│   ├── main.cpp            # Punto de entrada principal
│   ├── mqtt_secure.cpp     # Implementación MQTT sobre WebSocket Secure (WSS)
│   ├── mqtt_secure.h       # Header del módulo MQTT
│   ├── rele_entrada.cpp    # Control de relé
│   ├── rele_entrada.h      # Header del módulo relé
│   ├── sensor_temp.cpp     # Lectura de sensor de temperatura DS18B20
│   ├── sensor_temp.h       # Header del módulo sensor
│   ├── wifi_net.cpp        # Gestión WiFi con WiFiManager
│   └── wifi_net.h          # Header del módulo WiFi
└── test/
    └── README
```

---

## 🎯 Descripción General

Este proyecto es un **firmware IoT modular** para **ESP32** que:

1. **Conecta a WiFi** usando WiFiManager (portal cautivo para configuración)
2. **Se comunica vía MQTT sobre WebSocket Secure (WSS) en puerto 443**
3. **Controla un relé** mediante comandos MQTT
4. **Lee temperatura** de un sensor DS18B20 y la publica vía MQTT

### Arquitectura de Comunicación
```
ESP32 <--WSS/TLS--> Nginx (Reverse Proxy) <---> Mosquitto MQTT Broker
         Puerto 443        gaddbar.duckdns.org
```

---

## 📦 Dependencias y Configuración

### `platformio.ini`

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    tzapu/WiFiManager @ ^2.0.17
    links2004/WebSockets @ ^2.4.1   ; Implementación robusta de RFC 6455
    hideakitai/MQTTPubSubClient @ ^0.3.2 ; Cliente MQTT moderno
    paulstoffregen/OneWire @ ^2.3.8
    milesburton/DallasTemperature @ ^3.11.0
```

### Librerías Utilizadas

| Librería | Versión | Propósito |
|----------|---------|-----------|
| WiFiManager | ^2.0.17 | Portal cautivo para configurar WiFi |
| WebSockets | ^2.4.1 | Cliente WebSocket para transporte MQTT |
| MQTTPubSubClient | ^0.3.2 | Cliente MQTT moderno compatible con WebSockets |
| OneWire | ^2.3.8 | Protocolo 1-Wire para DS18B20 |
| DallasTemperature | ^3.11.0 | Lectura de sensores DS18B20 |

---

## 📄 Código Fuente Completo

### `src/config.h` - Configuración Global

```cpp
#pragma once

// --- Servidor ---
// Agregamos 'static' para evitar el error de "multiple definition"
static const char* MQTT_HOST = "gaddbar.duckdns.org"; 
static const int   MQTT_PORT = 443;
static const char* MQTT_PATH = "/mqtt";
static const char* MQTT_USER = "admin";
static const char* MQTT_PASS = "17203451Lili="; 

// --- Pines ---
static const int PIN_DS18B20 = 4;
static const int PIN_RELE    = 5;

// --- Tópicos ---
static const char* TOPIC_TEMP       = "bar/sensor/temp";
static const char* TOPIC_RELE_CMD   = "bar/rele/cmd";
static const char* TOPIC_RELE_STATE = "bar/rele/state";
```

**Notas importantes:**
- Las variables usan `static` para evitar errores de "multiple definition" al incluir en múltiples archivos
- `MQTT_PATH = "/mqtt"` es la ruta configurada en Nginx para el proxy WebSocket
- Puerto 443 = HTTPS/WSS estándar con TLS

---

### `src/main.cpp` - Punto de Entrada

```cpp
/**
 * @file main.cpp
 * @brief Firmware Modular Bar Gadd IoT - WSS Over 443
 * @author Gadd IoT Architect
 */
#include <Arduino.h>
#include "config.h"
#include "wifi_net.h"
#include "mqtt_secure.h"
#include "rele_entrada.h"
#include "sensor_temp.h"

// Instancias de Módulos
ReleEntrada rele;
SensorTemp sensor;

// --- Callback: Qué hacer cuando llega un mensaje MQTT ---
void alRecibirComando(String topic, String payload) {
    Serial.printf("[MSG] %s: %s\n", topic.c_str(), payload.c_str());

    if (topic == TOPIC_RELE_CMD) {
        if (payload == "ON" || payload == "1") {
            rele.encender();
            mqttPublish(TOPIC_RELE_STATE, "ON");
        } 
        else if (payload == "OFF" || payload == "0") {
            rele.apagar();
            mqttPublish(TOPIC_RELE_STATE, "OFF");
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    // 1. Inicializar Hardware
    rele.init(PIN_RELE);
    sensor.init(PIN_DS18B20);

    // 2. Conectar WiFi
    setupWiFi();

    // 3. Iniciar Sistema Seguro (Pasa el callback como argumento)
    setupMqttSecure(alRecibirComando);
}

void loop() {
    // Ciclos vitales de comunicación
    loopMqttSecure();

    // Tarea: Lectura de Sensores (No bloqueante)
    static unsigned long last_read = 0;
    if (millis() - last_read > 10000) { // Cada 10 segundos
        last_read = millis();
        
        float temp = sensor.leer();
        
        // CORRECCIÓN AQUÍ: Usamos el nombre correcto de la función
        // Antes decía isMqttConnected() y fallaba
        if (temp > -100 && mqttIsConnected()) {
            mqttPublish(TOPIC_TEMP, String(temp));
            Serial.printf("[TELEMETRIA] Temp enviada: %.2f C\n", temp);
        }
    }
}
```

**Flujo de ejecución:**
1. `setup()`: Inicializa hardware → Conecta WiFi → Inicia MQTT seguro
2. `loop()`: Mantiene conexión MQTT + Publica temperatura cada 10 segundos

---

### `src/wifi_net.h` - Header WiFi

```cpp
#pragma once
#include <WiFiManager.h>

void setupWiFi();
```

### `src/wifi_net.cpp` - Implementación WiFi

```cpp
#include "wifi_net.h"

void setupWiFi() {
    WiFiManager wm;
    // Callback opcional para cuando entra en modo AP
    // wm.setAPCallback(...); 

    // Intenta conectar. Si falla, levanta AP "Gadd_Setup" sin clave (o poné clave en 2do param)
    if (!wm.autoConnect("Gadd_Setup")) {
        Serial.println("[WIFI] Falló la conexión, reiniciando...");
        delay(3000);
        ESP.restart();
    }
    Serial.println("[WIFI] Conectado exitosamente!");
    Serial.print("[WIFI] IP: ");
    Serial.println(WiFi.localIP());
}
```

**Comportamiento:**
- Si no hay credenciales guardadas, crea un AP llamado "Gadd_Setup"
- El usuario se conecta al AP y configura el WiFi desde un portal web
- Las credenciales se guardan en la memoria NVS del ESP32

---

### `src/mqtt_secure.h` - Header MQTT Seguro

```cpp
#pragma once
#include <Arduino.h>

// Definimos un tipo de función para el callback
typedef std::function<void(String, String)> MqttCallback;

void setupMqttSecure(MqttCallback callback);
void loopMqttSecure();
void mqttPublish(const char* topic, String payload);

// Corrección: Agregamos esta declaración que faltaba
bool mqttIsConnected();
```

### `src/mqtt_secure.cpp` - Implementación MQTT sobre WSS

```cpp
#include "mqtt_secure.h"
#include "config.h"
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>
#include <MQTTPubSubClient.h>
#include "time.h"
#include <esp_crt_bundle.h>

// --- FIX CRÍTICO: Declaración del puntero al bundle de certificados ---
// Esto le dice al compilador dónde empieza el archivo binario de certificados en la memoria flash
extern const uint8_t rootca_crt_bundle_start[] asm("_binary_x509_crt_bundle_start");

// Instancias Globales
WiFiClientSecure clientSecure;
WebSocketsClient wsClient;
MQTTPubSubClient mqtt;

// Variable para guardar el callback del usuario y usarlo en reconexiones
MqttCallback globalUserCallback = nullptr;

// Variable para controlar el tiempo de reconexión
unsigned long last_mqtt_retry = 0;

// --- FUNCIONES AUXILIARES ---

void syncTime() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.print("[NTP] Sincronizando hora");
    
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)) {
        Serial.print(".");
        delay(100);
    }
    Serial.println("\n[NTP] Hora sincronizada OK.");
}

void onWSEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.println("[WSS] Desconectado.");
            break;
        case WStype_CONNECTED:
            Serial.println("[WSS] Conectado (Túnel OK).");
            break;
        case WStype_ERROR:
            Serial.println("[WSS] Error.");
            break;
        default:
            break;
    }
}

// Helper para suscribirse usando el callback global
void suscribirseAComandos() {
    // La librería exige: subscribe(TOPIC, CALLBACK)
    mqtt.subscribe(TOPIC_RELE_CMD, [](const String& payload, const size_t size) {
        if (globalUserCallback) {
            globalUserCallback(TOPIC_RELE_CMD, payload);
        }
    });
}

// --- FUNCIONES PRINCIPALES ---

void setupMqttSecure(MqttCallback msgCallback) {
    // Guardamos la función que vino de main.cpp
    globalUserCallback = msgCallback;

    // 1. Hora (Vital para SSL)
    syncTime();

    // 2. Certificados (Bundle de Mozilla)
    // Usamos el puntero declarado arriba (rootca_crt_bundle_start)
    clientSecure.setCACertBundle(rootca_crt_bundle_start);
    

    // 3. WebSocket Secure
    // "mqtt" es el subprotocolo requerido por Mosquitto
    wsClient.beginSSL(MQTT_HOST, MQTT_PORT, MQTT_PATH, "", "mqtt");
    wsClient.onEvent(onWSEvent);
    wsClient.setReconnectInterval(5000);

    // 4. Iniciar MQTT
    mqtt.begin(wsClient);
    
    // Callback por defecto para tópicos desconocidos
    mqtt.subscribe([](const String& topic, const String& payload, const size_t size) {
        Serial.println("[MQTT] Mensaje desconocido: " + topic);
    });
}

void loopMqttSecure() {
    wsClient.loop(); 
    mqtt.update();   

    // Lógica de Reconexión
    if (wsClient.isConnected() && !mqtt.isConnected()) {
        if (millis() - last_mqtt_retry > 5000) {
            last_mqtt_retry = millis();
            Serial.print("[MQTT] Reconectando...");
            
            if (mqtt.connect("ESP32_Bar_Gadd_Rele", MQTT_USER, MQTT_PASS)) {
                Serial.println(" OK!");
                suscribirseAComandos(); 
            } else {
                Serial.println(" Falló.");
            }
        }
    }
}

void mqttPublish(const char* topic, String payload) {
    if (mqtt.isConnected()) {
        mqtt.publish(topic, payload);
    }
}

bool mqttIsConnected() {
    return mqtt.isConnected();
}
```

**Puntos clave:**
- Usa el **bundle de certificados CA de Mozilla** incluido en ESP-IDF
- Sincroniza hora vía NTP (requerido para validar certificados SSL)
- WebSocket con subprotocolo "mqtt" para compatibilidad con Mosquitto
- Reconexión automática cada 5 segundos si se pierde la conexión

---

### `src/rele_entrada.h` - Header Relé

```cpp
#pragma once
#include <Arduino.h>

class ReleEntrada {
    public:
        // Corrección: init ahora recibe el pin
        void init(int pin);
        void encender();
        void apagar();
        bool getEstado();
    private:
        int _pin;
        bool _estado;
};
```

### `src/rele_entrada.cpp` - Implementación Relé

```cpp
#include "rele_entrada.h"

void ReleEntrada::init(int pin) {
    _pin = pin;
    pinMode(_pin, OUTPUT);
    apagar(); // Seguridad: siempre arrancar apagado
}

void ReleEntrada::encender() {
    digitalWrite(_pin, HIGH);
    _estado = true;
    Serial.println("[RELE] -> ON");
}

void ReleEntrada::apagar() {
    digitalWrite(_pin, LOW);
    _estado = false;
    Serial.println("[RELE] -> OFF");
}

bool ReleEntrada::getEstado() {
    return _estado;
}
```

---

### `src/sensor_temp.h` - Header Sensor Temperatura

```cpp
#pragma once
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

class SensorTemp {
    public:
        SensorTemp(); 
        // Corrección: init ahora recibe el pin
        void init(int pin);
        float leer();
    private:
        OneWire* _oneWire;
        DallasTemperature* _sensors;
};
```

### `src/sensor_temp.cpp` - Implementación Sensor Temperatura

```cpp
#include "sensor_temp.h"

SensorTemp::SensorTemp() {
    _oneWire = nullptr;
    _sensors = nullptr;
}

void SensorTemp::init(int pin) {
    // Inicializamos OneWire con el pin que nos manda main.cpp
    _oneWire = new OneWire(pin);
    _sensors = new DallasTemperature(_oneWire);
    _sensors->begin();
}

float SensorTemp::leer() {
    if (!_sensors) return -127;
    _sensors->requestTemperatures();
    float temp = _sensors->getTempCByIndex(0);
    if (temp == DEVICE_DISCONNECTED_C) {
        Serial.println("[TEMP] Error: Sensor desconectado");
        return -127;
    }
    return temp;
}
```

---

## 🔌 Hardware Requerido

| Componente | Pin ESP32 | Descripción |
|------------|-----------|-------------|
| Sensor DS18B20 | GPIO 4 | Sensor de temperatura 1-Wire |
| Módulo Relé | GPIO 5 | Control de dispositivo ON/OFF |

### Diagrama de Conexiones

```
ESP32 DevKit V1
    │
    ├── GPIO 4 ──── DATA (DS18B20)
    │                 │
    │                4.7kΩ (pull-up a 3.3V)
    │
    ├── GPIO 5 ──── IN (Módulo Relé)
    │
    ├── 3.3V ────── VCC (DS18B20)
    │
    ├── 5V ──────── VCC (Módulo Relé)
    │
    └── GND ─────── GND (ambos)
```

---

## 📡 Tópicos MQTT

| Tópico | Dirección | Payload | Descripción |
|--------|-----------|---------|-------------|
| `bar/sensor/temp` | ESP32 → Broker | `23.5` | Temperatura en °C |
| `bar/rele/cmd` | Broker → ESP32 | `ON`, `OFF`, `1`, `0` | Comando para el relé |
| `bar/rele/state` | ESP32 → Broker | `ON`, `OFF` | Estado actual del relé |

---

## 🖥️ Configuración del Servidor

### Requisitos del Servidor
- **Mosquitto** con WebSockets habilitado
- **Nginx** como reverse proxy TLS (Let's Encrypt)
- **DuckDNS** para dominio dinámico gratuito

### Ejemplo configuración Nginx (`/etc/nginx/sites-available/default`)

```nginx
server {
    listen 443 ssl;
    server_name gaddbar.duckdns.org;

    ssl_certificate /etc/letsencrypt/live/gaddbar.duckdns.org/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/gaddbar.duckdns.org/privkey.pem;

    location /mqtt {
        proxy_pass http://127.0.0.1:9001;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host $host;
        proxy_read_timeout 86400;
    }
}
```

### Ejemplo configuración Mosquitto (`/etc/mosquitto/mosquitto.conf`)

```conf
listener 1883
listener 9001
protocol websockets

allow_anonymous false
password_file /etc/mosquitto/passwd
```

---

## 🚀 Instrucciones de Compilación

```bash
# Compilar
pio run

# Subir al ESP32
pio run --target upload

# Monitor Serial
pio device monitor
```

---

## 🔧 Solución de Problemas Comunes

| Problema | Causa | Solución |
|----------|-------|----------|
| `multiple definition` | Variables globales en header | Usar `static` en `config.h` |
| `mqttIsConnected` no encontrada | Falta declaración | Agregar en `mqtt_secure.h` |
| No conecta SSL | Hora no sincronizada | Verificar NTP y certificados |
| WebSocket desconecta | Timeout del servidor | Ajustar `proxy_read_timeout` |

---

## 📝 Notas Técnicas

1. **Bundle de Certificados**: El ESP-IDF incluye un bundle de certificados CA de Mozilla. Se accede mediante:
   ```cpp
   extern const uint8_t rootca_crt_bundle_start[] asm("_binary_x509_crt_bundle_start");
   ```

2. **Subprotocolo WebSocket**: Mosquitto requiere el subprotocolo `"mqtt"` para conexiones WebSocket.

3. **NTP**: La validación de certificados SSL requiere hora correcta. Siempre sincronizar antes de conectar.

4. **Reconexión**: El sistema reintenta conexión MQTT cada 5 segundos si el WebSocket está activo pero MQTT desconectado.

---

## 📜 Licencia

Proyecto desarrollado para Gadd IoT - Uso interno.

---

*Documento generado el 4 de diciembre de 2025*
