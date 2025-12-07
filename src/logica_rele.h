/**
 * @file logica_rele.h
 * @brief Desacopla la lógica de negocio del loop principal.
 */
#pragma once
#include <Arduino.h>
#include "rele_entrada.h" 

// Procesa el comando MQTT y ejecuta la Estrategia C (Acción + Feedback)
void procesarComandoRele(String payload, ReleEntrada &rele);
