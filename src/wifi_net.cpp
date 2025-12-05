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