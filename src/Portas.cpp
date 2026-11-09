#include "Portas.h"
#include "DingDong.h"
#include <Arduino.h>
#include <Servo.h>

// MAPA DE PINOS REAIS DO HARDWARE
#define SENSOR_IV_PIN 8   // Pino do Sensor IR de Presença

#define SERVO_P1 A0       // Servo Porta 1º Andar
#define SERVO_P2 A1       // Servo Porta 2º Andar
#define SERVO_P3 A2       // Servo Porta 3º Andar

// ============================================================================
// 🎯 CALIBRAÇÃO MECÂNICA DOS ÂNGULOS POR ANDAR
// ============================================================================
#define ANGULO_FECHADO_P1  5  // Calibração de fecho do 1º andar
#define ANGULO_ABERTO_P1   95  // Calibração de abertura do 1º andar

#define ANGULO_FECHADO_P2  16   // Calibração do 2º andar
#define ANGULO_ABERTO_P2   106

#define ANGULO_FECHADO_P3  5   // Calibração do 3º andar
#define ANGULO_ABERTO_P3   95

// TEMPO DE MOVIMENTO (2400ms = metade da velocidade original / movimento suave)
#define TEMPO_MOVIMENTO 1500 
// ============================================================================

enum EstadoPorta {
  PORTA_FECHADA,
  PORTA_ABRINDO,
  PORTA_ABERTA,
  PORTA_FECHANDO
};

static EstadoPorta estadoAtualPorta = PORTA_FECHADA;
static int andarAtualPorta = 1;
static unsigned long tempoInicioEstado = 0;
static unsigned long tempoInicioObstrucao = 0;

Servo servoP1;
Servo servoP2;
Servo servoP3;

Servo* getServoAndar(int andar) {
  if (andar == 1) return &servoP1;
  if (andar == 2) return &servoP2;
  if (andar == 3) return &servoP3;
  return &servoP1;
}

int getAnguloFechado(int andar) {
  if (andar == 1) return ANGULO_FECHADO_P1;
  if (andar == 2) return ANGULO_FECHADO_P2;
  if (andar == 3) return ANGULO_FECHADO_P3;
  return ANGULO_FECHADO_P1;
}

int getAnguloAberto(int andar) {
  if (andar == 1) return ANGULO_ABERTO_P1;
  if (andar == 2) return ANGULO_ABERTO_P2;
  if (andar == 3) return ANGULO_ABERTO_P3;
  return ANGULO_ABERTO_P1;
}

bool sensorObstaculoAtivo() {
  return (digitalRead(SENSOR_IV_PIN) == LOW);
}

bool lerSensorIR() {
  return sensorObstaculoAtivo();
}

void inicializarPortas() {
  pinMode(SENSOR_IV_PIN, INPUT_PULLUP);

  servoP1.attach(SERVO_P1, 500, 2500);
  servoP2.attach(SERVO_P2, 500, 2500);
  servoP3.attach(SERVO_P3, 500, 2500);

  servoP1.write(getAnguloFechado(1));
  servoP2.write(getAnguloFechado(2));
  servoP3.write(getAnguloFechado(3));

  delay(300);

  // Desconecta os servos para impedir trepidações por ruído elétrico durante o Homing
  servoP1.detach();
  servoP2.detach();
  servoP3.detach();

  estadoAtualPorta = PORTA_FECHADA;
}

void comandarAberturaPorta(int andar) {
  andarAtualPorta = andar;
  estadoAtualPorta = PORTA_ABRINDO;
  tempoInicioEstado = millis();
  tempoInicioObstrucao = 0;

  Servo* s = getServoAndar(andarAtualPorta);
  int pino = (andar == 1 ? SERVO_P1 : (andar == 2 ? SERVO_P2 : SERVO_P3));
  if (!s->attached()) {
    s->attach(pino, 500, 2500);
  }
}

void gerenciarMaquinaPortas() {
  unsigned long agora = millis();
  bool haObstaculo = sensorObstaculoAtivo();

  int anguloFechadoAndar = getAnguloFechado(andarAtualPorta);
  int anguloAbertoAndar  = getAnguloAberto(andarAtualPorta);

  switch (estadoAtualPorta) {
    
    case PORTA_FECHADA:
      setAlertaObstrucao(false);
      tempoInicioObstrucao = 0;
      break;

    case PORTA_ABRINDO: {
      unsigned long decorrido = agora - tempoInicioEstado;
      if (decorrido > TEMPO_MOVIMENTO) decorrido = TEMPO_MOVIMENTO;
      
      int angulo = map(decorrido, 0, TEMPO_MOVIMENTO, anguloFechadoAndar, anguloAbertoAndar);
      getServoAndar(andarAtualPorta)->write(angulo);

      if (decorrido >= TEMPO_MOVIMENTO) { 
        estadoAtualPorta = PORTA_ABERTA;
        tempoInicioEstado = agora;
        tempoInicioObstrucao = 0;
      }
      break;
    }

    case PORTA_ABERTA:
      if (haObstaculo) {
        tempoInicioEstado = agora; 

        if (tempoInicioObstrucao == 0) {
          tempoInicioObstrucao = agora;
        }

        if (agora - tempoInicioObstrucao >= 4000) {
          setAlertaObstrucao(true);
        }
      } else {
        tempoInicioObstrucao = 0;
        setAlertaObstrucao(false);

        if (agora - tempoInicioEstado >= 6000) { // Tempo em que a porta permanece aberta
          estadoAtualPorta = PORTA_FECHANDO;
          tempoInicioEstado = agora;
        }
      }
      break;

    case PORTA_FECHANDO: {
      unsigned long decorrido = agora - tempoInicioEstado;
      if (decorrido > TEMPO_MOVIMENTO) decorrido = TEMPO_MOVIMENTO;

      int angulo = map(decorrido, 0, TEMPO_MOVIMENTO, anguloAbertoAndar, anguloFechadoAndar);
      getServoAndar(andarAtualPorta)->write(angulo);

      // REABERTURA INSTANTÂNEA AO DETECTAR OBSTÁCULO NO IR
      if (haObstaculo) {
        // Calcula a posição atual e mapeia para o tempo equivalente de abertura
        unsigned long decorridoAbrindoEquiv = map(angulo, anguloFechadoAndar, anguloAbertoAndar, 0, TEMPO_MOVIMENTO);

        // Inverte a máquina de estados imediatamente sem dar tranco nem fechar mais nada
        estadoAtualPorta = PORTA_ABRINDO;
        tempoInicioEstado = agora - decorridoAbrindoEquiv;
        tempoInicioObstrucao = agora;
        setAlertaObstrucao(false);
        break;
      }

      if (decorrido >= TEMPO_MOVIMENTO) { 
        getServoAndar(andarAtualPorta)->write(anguloFechadoAndar);
        estadoAtualPorta = PORTA_FECHADA;
        setAlertaObstrucao(false);
        tempoInicioObstrucao = 0;

        // Desconecta o sinal do servo ao fechar 100% para evitar ruído e movimentos involuntários
        getServoAndar(andarAtualPorta)->detach();
      }
      break;
    }
  }
}

bool portaEstaTotalmenteFechada() {
  return (estadoAtualPorta == PORTA_FECHADA);
}

bool portaEstaAberta() {
  return (estadoAtualPorta != PORTA_FECHADA);
}

void ativarPortaEmergencia(int andar) {
  setAlertaObstrucao(false);
  andarAtualPorta = andar;
  Servo* s = getServoAndar(andar);
  int pino = (andar == 1 ? SERVO_P1 : (andar == 2 ? SERVO_P2 : SERVO_P3));
  
  if (!s->attached()) {
    s->attach(pino, 500, 2500);
  }
  
  s->write(getAnguloAberto(andar));
  delay(500);
  s->detach();
}

void restaurarPortasAposEmergencia(int andar) {
  andarAtualPorta = andar;
  Servo* s = getServoAndar(andar);
  int pino = (andar == 1 ? SERVO_P1 : (andar == 2 ? SERVO_P2 : SERVO_P3));
  
  if (!s->attached()) {
    s->attach(pino, 500, 2500);
  }

  // Posiciona em aberto e inicia o fechamento progressivo e suave (2400ms)
  s->write(getAnguloAberto(andar));
  estadoAtualPorta = PORTA_FECHANDO;
  tempoInicioEstado = millis();
  setAlertaObstrucao(false);
}