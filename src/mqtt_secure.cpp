/**
 * @file mqtt_secure.cpp
 * @brief Implementación de MQTT sobre WSS con Last Will (LWT) - CORREGIDO
 */

#include "mqtt_secure.h"
#include "config.h"
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>
#include <MQTTPubSubClient.h>
#include "time.h"
#include <esp_crt_bundle.h> 

extern const uint8_t rootca_crt_bundle_start[] asm("_binary_x509_crt_bundle_start");

// Instancias Globales
WiFiClientSecure clientSecure;
WebSocketsClient wsClient;
MQTTPubSubClient mqtt;

MqttCallback globalUserCallback = nullptr;
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
            Serial.println("[WSS] Túnel Seguro Establecido.");
            break;
        case WStype_ERROR:
            Serial.println("[WSS] Error.");
            break;
        default:
            break;
    }
}

void suscribirseAComandos() {
    mqtt.subscribe(TOPIC_RELE_CMD, [](const String& payload, const size_t size) {
        if (globalUserCallback) {
            globalUserCallback(TOPIC_RELE_CMD, payload);
        }
    });
}

// --- FUNCIONES PRINCIPALES ---

void setupMqttSecure(MqttCallback msgCallback) {
    globalUserCallback = msgCallback;

    syncTime();
    clientSecure.setCACertBundle(rootca_crt_bundle_start);

    wsClient.beginSSL(MQTT_HOST, MQTT_PORT, MQTT_PATH, "", "mqtt");
    wsClient.onEvent(onWSEvent);
    wsClient.setReconnectInterval(5000);

    mqtt.begin(wsClient);
    
    // Callback vacío para evitar ruido en logs
    mqtt.subscribe([](const String& topic, const String& payload, const size_t size) {});
}

void loopMqttSecure() {
    wsClient.loop(); 
    mqtt.update();   

    if (wsClient.isConnected() && !mqtt.isConnected()) {
        if (millis() - last_mqtt_retry > 5000) {
            last_mqtt_retry = millis();
            Serial.print("[MQTT] Conectando... ");
            
            // --- CORRECCIÓN AQUÍ ---
            // 1. Configuramos la Última Voluntad (LWT) ANTES de conectar
            // setWill(topic, payload, retained, qos)
            mqtt.setWill(TOPIC_STATUS, "OFFLINE", true, 1);

            // 2. Conectamos usando solo ID, Usuario y Clave
            if (mqtt.connect("ESP32_Bar_Gadd_Rele", MQTT_USER, MQTT_PASS)) {
                
                Serial.println("OK!");
                
                // 3. Publicamos que estamos ONLINE (Retenido = true)
                mqtt.publish(TOPIC_STATUS, "ONLINE", true);
                
                suscribirseAComandos(); 
            } else {
                Serial.println("Falló.");
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