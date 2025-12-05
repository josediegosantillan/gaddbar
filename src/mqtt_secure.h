#pragma once
#include <Arduino.h>

// Definimos un tipo de función para el callback
typedef std::function<void(String, String)> MqttCallback;

void setupMqttSecure(MqttCallback callback);
void loopMqttSecure();
void mqttPublish(const char* topic, String payload);

// Corrección: Agregamos esta declaración que faltaba
bool mqttIsConnected();