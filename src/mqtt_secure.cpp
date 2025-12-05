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