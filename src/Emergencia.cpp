#include "Emergencia.h"
#include "Display.h"
#include "Motor.h"
#include "DingDong.h"
#include <Arduino.h>

#define BtEmergencia 2
#define BtReset 3

volatile bool emergenciaAtivada = false;

volatile unsigned long ultimoTempoEmergencia = 0;
volatile unsigned long ultimoTempoReset = 0;
const unsigned long TEMPO_DEBOUNCE = 150; 

void EmergenciaInit() {
  pinMode(BtEmergencia, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BtEmergencia), ParadaEmergencia, FALLING);
  
  pinMode(BtReset, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BtReset), ResetEmergencia, FALLING);
}

void ParadaEmergencia() { 
  unsigned long tempoAtual = millis();
  if (tempoAtual - ultimoTempoEmergencia > TEMPO_DEBOUNCE) {
    if (digitalRead(BtEmergencia) == LOW) { 
      emergenciaAtivada = true; 
      ultimoTempoEmergencia = tempoAtual;
    }
  }
}

void ResetEmergencia() {
  unsigned long tempoAtual = millis();
  if (tempoAtual - ultimoTempoReset > TEMPO_DEBOUNCE) {
    // CORREÇÃO DO AUTO-RESET / BYPASS:
    // Só reseta se o botão FÍSICO de Reset estiver REALMENTE pressionado (LOW)
    // E o botão físico de Emergência estiver destravado/solto (HIGH)
    if (digitalRead(BtReset) == LOW && digitalRead(BtEmergencia) == HIGH) { 
      emergenciaAtivada = false;
      Serial.println("E:0");
      ultimoTempoReset = tempoAtual;
    }
  }
}

// RESET VIA SOFTWARE (HTML / BLUETOOTH)
bool tentarResetSoftware() {
  // Permite o reset via software apenas se o botão físico de emergência NÃO estiver retido
  if (digitalRead(BtEmergencia) == HIGH) {
    emergenciaAtivada = false;
    return true;
  }
  return false;
}

void rotinaSeguranca() {
  pararMotor();             
  setSireneEmergencia(true);  
  lcdEmergencia();           
  Serial.println("E:1"); 
}