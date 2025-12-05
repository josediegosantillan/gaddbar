#pragma once
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

class SensorTemp {
    public:
        SensorTemp(); 
        // Corrección: init ahora recibe el pin
        void init(int pin);
        float leer();
    private:
        OneWire* _oneWire;
        DallasTemperature* _sensors;
};
