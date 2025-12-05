#pragma once
#include <Arduino.h>

class ReleEntrada {
    public:
        // Corrección: init ahora recibe el pin
        void init(int pin);
        void encender();
        void apagar();
        bool getEstado();
    private:
        int _pin;
        bool _estado;
};