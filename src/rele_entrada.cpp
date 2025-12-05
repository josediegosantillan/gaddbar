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