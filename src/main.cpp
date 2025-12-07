/**
 * @file main.cpp
 * @brief Firmware Modular Bar Gadd IoT - WSS Over 443
 */
#include <Arduino.h>
#include "config.h"
#include "wifi_net.h"
#include "mqtt_secure.h"
#include "rele_entrada.h"
#include "sensor_temp.h"
#include "logica_rele.h" // <--- IMPORTANTE: Incluir el nuevo módulo

// Instancias de Módulos
ReleEntrada rele;
SensorTemp sensor;

// --- Callback: Qué hacer cuando llega un mensaje MQTT ---
void alRecibirComando(String topic, String payload) {
    Serial.printf("[MSG] Tópico: %s | Carga: %s\n", topic.c_str(), payload.c_str());

    // Ruteo de mensajes
    if (topic == TOPIC_RELE_CMD) {
        // Delegamos la lógica al módulo especializado
        // Pasamos el objeto 'rele' por referencia para que el módulo pueda controlarlo
        procesarComandoRele(payload, rele);
    }
    // Aquí podrías agregar más 'else if' para otros tópicos futuros
}

void setup() {
    Serial.begin(115200);
    
    // 1. Inicializar Hardware
    rele.init(PIN_RELE);     
    sensor.init(PIN_DS18B20); 

    // 2. Conectar WiFi
    setupWiFi();

    // 3. Iniciar Sistema Seguro
    setupMqttSecure(alRecibirComando);
}

void loop() {
    loopMqttSecure();

    static unsigned long last_read = 0;
    if (millis() - last_read > 10000) { 
        last_read = millis();
        float temp = sensor.leer();
        
        if (temp > -100 && mqttIsConnected()) {
            mqttPublish(TOPIC_TEMP, String(temp));
            Serial.printf("[TELEMETRIA] Temp enviada: %.2f C\n", temp);
        }
    }
}
