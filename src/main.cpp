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
