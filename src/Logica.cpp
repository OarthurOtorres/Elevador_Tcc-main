#include "Logica.h"
#include "Motor.h"
#include "Display.h"   // Incluído para controlar a tela no Homing
#include "Bluetooth.h" // Incluído para enviar log no Homing
#include "Portas.h"    // Incluído para controle de abertura de porta no mesmo andar
#include <Arduino.h>
#include <Wire.h>

#define PCF_ADDR 0x27

#define S1 5
#define S2 6
#define S3 7

bool chamada[4] = {false, false, false, false};

int andarAtual = 0;
int andarDestino = 0;
int estado = 0;
int direcaoAtual = 0;

unsigned int totalViagensAcumuladas = 0;

void registrarViagemConcluida() {
  totalViagensAcumuladas++;
}

bool lerSensorComFiltro(int pino) {
  int contagemLow = 0;
  for (int i = 0; i < 5; i++) {
    if (digitalRead(pino) == LOW) {
      contagemLow++;
    }
    delay(10);
  }
  return (contagemLow >= 4);
}

bool sensorAtivo(int andar) {
  if (andar == 1) return digitalRead(S1) == LOW;
  if (andar == 2) return digitalRead(S2) == LOW;
  if (andar == 3) return digitalRead(S3) == LOW;
  return false;
}

void executarHoming() {
  lcdReferenciando();
  enviarLog("Elevador em Referenciamento...");
  
  ligarMotorDescer();
  
  while (!sensorAtivo(1) && !sensorAtivo(2) && !sensorAtivo(3)) {
    atualizarRampaMotor();
    delay(10);
  }
  
  pararMotor();

  if (sensorAtivo(1)) andarAtual = 1;
  else if (sensorAtivo(2)) andarAtual = 2;
  else if (sensorAtivo(3)) andarAtual = 3;

  andarDestino = andarAtual;

  lcdParado();
  enviarLog("Elevador Parado no " + String(andarAtual) + "º Andar");
}

void detectarAndarInicial() {
  delay(200);

  if (lerSensorComFiltro(S1)) {
    andarAtual = 1;
    andarDestino = 1;
  } else if (lerSensorComFiltro(S2)) {
    andarAtual = 2;
    andarDestino = 2;
  } else if (lerSensorComFiltro(S3)) {
    andarAtual = 3;
    andarDestino = 3;
  } else {
    executarHoming();
  }
}

void initBtsESensores() {
  pinMode(S1, INPUT_PULLUP);
  pinMode(S2, INPUT_PULLUP);
  pinMode(S3, INPUT_PULLUP);

  Wire.begin();

  Wire.beginTransmission(PCF_ADDR);
  Wire.write(0b01101101);
  Wire.endTransmission();

  detectarAndarInicial();
}

void atualizarLedsBotoes() {
  for (int i = 1; i <= 3; i++) {
    if (sensorAtivo(i)) {
      chamada[i] = false;
    }
  }

  uint8_t bytePCF = 0b01101101; 

  if (chamada[1]) bytePCF |= (1 << 1);
  if (chamada[2]) bytePCF |= (1 << 4);
  if (chamada[3]) bytePCF |= (1 << 7);

  Wire.beginTransmission(PCF_ADDR);
  Wire.write(bytePCF);
  Wire.endTransmission();
}

void lerBotoes() {
  Wire.requestFrom((uint8_t)PCF_ADDR, (uint8_t)1);
  if (!Wire.available()) return;

  uint8_t leitura = Wire.read();

  bool bt1Pressionado = !(leitura & (1 << 0));
  bool bt2Pressionado = !(leitura & (1 << 3));
  bool bt3Pressionado = !(leitura & (1 << 6));

  // --- BOTÃO ANDAR 1 ---
  if (bt1Pressionado) {
    if (andarAtual == 1 && estado == 0 && portaEstaTotalmenteFechada()) {
      comandarAberturaPorta(1); // Se já está no andar 1 em standby, reabre a porta
    } else if (!sensorAtivo(1)) {
      chamada[1] = true;
    }
  }

  // --- BOTÃO ANDAR 2 ---
  if (bt2Pressionado) {
    if (andarAtual == 2 && estado == 0 && portaEstaTotalmenteFechada()) {
      comandarAberturaPorta(2); // Se já está no andar 2 em standby, reabre a porta
    } else if (!sensorAtivo(2)) {
      chamada[2] = true;
    }
  }

  // --- BOTÃO ANDAR 3 ---
  if (bt3Pressionado) {
    if (andarAtual == 3 && estado == 0 && portaEstaTotalmenteFechada()) {
      comandarAberturaPorta(3); // Se já está no andar 3 em standby, reabre a porta
    } else if (!sensorAtivo(3)) {
      chamada[3] = true;
    }
  }

  atualizarLedsBotoes();
}

int escolherProximoAndar() {
  if (direcaoAtual == 1) {
    for (int i = andarAtual + 1; i <= 3; i++) {
      if (chamada[i]) return i;
    }
    direcaoAtual = -1;
  }

  if (direcaoAtual == -1) {
    for (int i = andarAtual - 1; i >= 1; i--) {
      if (chamada[i]) return i;
    }
    direcaoAtual = 0;
  }

  if (direcaoAtual == 0) {
    for (int i = andarAtual + 1; i <= 3; i++) {
      if (chamada[i]) {
        direcaoAtual = 1;
        return i;
      }
    }
    for (int i = andarAtual - 1; i >= 1; i--) {
      if (chamada[i]) {
        direcaoAtual = -1;
        return i;
      }
    }
  }

  return 0;
}