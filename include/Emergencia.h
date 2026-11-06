#ifndef EMERGENCIA_H
#define EMERGENCIA_H

#include <Arduino.h>

extern volatile bool emergenciaAtivada;

void EmergenciaInit();
void ParadaEmergencia();
void ResetEmergencia();
bool tentarResetSoftware(); // Função dedicada para o Reset via Web/Bluetooth
void rotinaSeguranca();

#endif