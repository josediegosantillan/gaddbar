#include "logica_rele.h"
#include "config.h"      
#include "mqtt_secure.h" 

void procesarComandoRele(String payload, ReleEntrada &rele) {
    
    // Normalizar entrada (evita errores por 'on' vs 'ON')
    payload.toUpperCase();

    // Lógica de Control
    if (payload == "ON" || payload == "1") {
        // 1. Acción Física (Hardware)
        rele.encender(); 
        
        // 2. Feedback Real (Estrategia C)
        // Solo publicamos el estado "ON" DESPUÉS de haber accionado el relé.
        // Esto actualiza el UI-Button en Node-RED.
        mqttPublish(TOPIC_RELE_STATE, "ON"); 
    } 
    else if (payload == "OFF" || payload == "0") {
        // 1. Acción Física
        rele.apagar();
        
        // 2. Feedback Real
        mqttPublish(TOPIC_RELE_STATE, "OFF");
    }
    else {
        Serial.printf("[LOGICA] Comando ignorado: %s\n", payload.c_str());
    }
}
