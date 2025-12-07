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
// NUEVO: Tópico de Presencia del Sistema
static const char* TOPIC_STATUS     = "bar/status";
