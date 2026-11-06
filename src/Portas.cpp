#include "Portas.h"
#include "DingDong.h"
#include <Arduino.h>
#include <Servo.h>

// MAPA DE PINOS REAIS DO HARDWARE
#define SENSOR_IV_PIN 8   // Pino do Sensor IR de Presença

#define SERVO_P1 A0       // Servo Porta 1º Andar
#define SERVO_P2 A1       // Servo Porta 2º Andar
#define SERVO_P3 A2       // Servo Porta 3º Andar


#define ANGULO_FECHADO 5  
#define ANGULO_ABERTO  95

// Tempo para abrir/fechar 
#define TEMPO_MOVIMENTO 1500 

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

bool sensorObstaculoAtivo() {
  return (digitalRead(SENSOR_IV_PIN) == LOW);
}

// Wrapper para o Bluetooth verificar o feixe IR
bool lerSensorIR() {
  return sensorObstaculoAtivo();
}

void inicializarPortas() {
  pinMode(SENSOR_IV_PIN, INPUT_PULLUP);

  servoP1.attach(SERVO_P1, 500, 2500);
  servoP2.attach(SERVO_P2, 500, 2500);
  servoP3.attach(SERVO_P3, 500, 2500);

  servoP1.write(ANGULO_FECHADO);
  servoP2.write(ANGULO_FECHADO);
  servoP3.write(ANGULO_FECHADO);

  estadoAtualPorta = PORTA_FECHADA;
}

void comandarAberturaPorta(int andar) {
  andarAtualPorta = andar;
  estadoAtualPorta = PORTA_ABRINDO;
  tempoInicioEstado = millis();
  tempoInicioObstrucao = 0;

  Servo* s = getServoAndar(andarAtualPorta);
  if (!s->attached()) {
    int pino = (andar == 1 ? SERVO_P1 : (andar == 2 ? SERVO_P2 : SERVO_P3));
    s->attach(pino, 500, 2500);
  }
}

void gerenciarMaquinaPortas() {
  unsigned long agora = millis();
  bool haObstaculo = sensorObstaculoAtivo();

  switch (estadoAtualPorta) {
    
    case PORTA_FECHADA:
      setAlertaObstrucao(false);
      tempoInicioObstrucao = 0;
      break;

    case PORTA_ABRINDO: {
      unsigned long decorrido = agora - tempoInicioEstado;
      if (decorrido > TEMPO_MOVIMENTO) decorrido = TEMPO_MOVIMENTO;
      
      // Movimento suave progressivo (Sem trancos)
      int angulo = map(decorrido, 0, TEMPO_MOVIMENTO, ANGULO_FECHADO, ANGULO_ABERTO);
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

        if (agora - tempoInicioEstado >= 6000) { // Tempo em que a porta fica aberta
          estadoAtualPorta = PORTA_FECHANDO;
          tempoInicioEstado = agora;
        }
      }
      break;

    case PORTA_FECHANDO: {
      unsigned long decorrido = agora - tempoInicioEstado;
      if (decorrido > TEMPO_MOVIMENTO) decorrido = TEMPO_MOVIMENTO;

      // Movimento suave progressivo de fechamento
      int angulo = map(decorrido, 0, TEMPO_MOVIMENTO, ANGULO_ABERTO, ANGULO_FECHADO);
      getServoAndar(andarAtualPorta)->write(angulo);

      if (haObstaculo) { // Reabertura de emergência se alguém passar na porta
        estadoAtualPorta = PORTA_ABRINDO;
        tempoInicioEstado = agora;
        tempoInicioObstrucao = agora;
        setAlertaObstrucao(false);
        break;
      }

      if (decorrido >= TEMPO_MOVIMENTO) { 
        estadoAtualPorta = PORTA_FECHADA;
        setAlertaObstrucao(false);
        tempoInicioObstrucao = 0;
      }
      break;
    }
  }
}

bool portaEstaTotalmenteFechada() {
  return (estadoAtualPorta == PORTA_FECHADA);
}

// Retorna verdadeiro para o Bluetooth caso QUALQUER servo esteja operando
bool portaEstaAberta() {
  return (estadoAtualPorta != PORTA_FECHADA);
}

void ativarPortaEmergencia(int andar) {
  setAlertaObstrucao(false);
  Servo* s = getServoAndar(andar);
  s->write(ANGULO_ABERTO);
  delay(500);
  s->detach();
}

void restaurarPortasAposEmergencia(int andar) {
  Servo* s = getServoAndar(andar);
  int pino = (andar == 1 ? SERVO_P1 : (andar == 2 ? SERVO_P2 : SERVO_P3));
  s->attach(pino, 500, 2500);
  s->write(ANGULO_FECHADO);
  estadoAtualPorta = PORTA_FECHADA;
}