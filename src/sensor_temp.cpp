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