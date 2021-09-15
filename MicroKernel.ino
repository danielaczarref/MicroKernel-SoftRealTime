#include <TimerOne.h>
#include "avr/wdt.h"
#include "DHT.h"

#define NUMBER_OF_TASKS  4
#define TEMPO_MAXIMO_EXECUCAO 100 
#define SUCCESS 1
#define FAIL    0
#define SIM 1
#define NAO 0

const int pino_dht = 9;
const int pinoLEDR = 11;
const int pinoLEDG = 12;
const int pinoLEDB = 7;
const int pinoBuzzer = 10;

float temperatura; 
float umidade;

DHT dht(pino_dht, DHT11);

typedef void(*ptrFunc)();
typedef struct{
  ptrFunc Function;
  uint32_t period;
  bool enableTask; 
} TaskHandle;


TaskHandle* buffer[NUMBER_OF_TASKS]; 

volatile uint32_t taskCounter[NUMBER_OF_TASKS];
volatile int16_t TempoEmExecucao;
volatile uint32_t sysTickCounter = 0;
volatile bool TemporizadorEstourou;
volatile bool TarefaSendoExecutada;
int contador = 0;
int ordem = 0;
int estado = 0;
const int pino_microfone = A2; 
int leitura = 0;
float tensao = 0.0;

char KernelInit()
{
  memset(buffer, NULL, sizeof(buffer));
  memset(taskCounter, 0, sizeof(taskCounter)); 

  TemporizadorEstourou = NAO;
  TarefaSendoExecutada = NAO;

  Timer1.initialize(10000);
  Timer1.attachInterrupt(IsrTimer);

  return SUCCESS;
}

char KernelAdd(TaskHandle* task)
{
  int i;
 
  for(i = 0; i < NUMBER_OF_TASKS; i++)
  {
    if((buffer[i]!=NULL) && (buffer[i] == task))
       return SUCCESS;
  }

  for(i = 0; i < NUMBER_OF_TASKS; i++)
  {
    if(!buffer[i])
    {
      buffer[i] = task;
      return SUCCESS;
    }
  }
  return FAIL;
}

char KernelRemoveTask(TaskHandle* task)
{
  int i;
  for(i=0; i<NUMBER_OF_TASKS; i++)
  {
     if(buffer[i] == task)
     {
        buffer[i] = NULL;
        return SUCCESS; 
     }
  }
  return FAIL;

}

void KernelLoop()
{
  int i;

  for (;;)
  {
    if (TemporizadorEstourou == SIM)
    {
      for (i = 0; i < NUMBER_OF_TASKS; i++)
      {
        if (buffer[i] != 0)
        {
          if (((sysTickCounter - taskCounter[i])>buffer[i]->period) && (buffer[i]->enableTask == SIM))
          {
            TarefaSendoExecutada = SIM;
            TempoEmExecucao = TEMPO_MAXIMO_EXECUCAO;
            buffer[i]->Function();
            TarefaSendoExecutada = NAO;
            taskCounter[i] = sysTickCounter;
          }
        }
      }
    }
  }
}

void IsrTimer()
{
  int i;
  TemporizadorEstourou = SIM;

  sysTickCounter++;
  
  if (TarefaSendoExecutada == SIM)
  {
    TempoEmExecucao--;
    if (!TempoEmExecucao)
    {
      wdt_enable(WDTO_15MS);
      while (1);
    }
  }
}

char proc1() {
  temperatura = dht.readTemperature();
  umidade = dht.readHumidity();

  if (isnan(umidade) || isnan(temperatura)) {
    // Acende branco
    digitalWrite(pinoLEDR, HIGH);
    digitalWrite(pinoLEDG, HIGH);
    digitalWrite(pinoLEDB, HIGH);
  }
}


char proc2() {
  temperatura = dht.readTemperature();
  umidade = dht.readHumidity();
  Serial.println();
  Serial.print("temperatura: ");
  Serial.print(temperatura);
  Serial.println();
  if (temperatura > 35 || temperatura < 15) {
      digitalWrite(pinoLEDR, HIGH);
      digitalWrite(pinoLEDG, LOW);
      digitalWrite(pinoLEDB, LOW);
      tone(pinoBuzzer, 2000);
      delay(1000);
  }
  else if (temperatura > 30 || temperatura < 20) {
      digitalWrite(pinoLEDR, LOW);
      digitalWrite(pinoLEDG, LOW);
      digitalWrite(pinoLEDB, HIGH);
      tone(pinoBuzzer, 1000);
      delay(500);
  }

  else {
      digitalWrite(pinoLEDR, LOW);
      digitalWrite(pinoLEDG, HIGH);
      digitalWrite(pinoLEDB, LOW);
  }

  noTone(pinoBuzzer);
}

char proc3() {
  leitura = analogRead(pino_microfone);
  Serial.println();
  Serial.print("som: ");
  Serial.print(leitura);
  Serial.println();
}


void setup() {
  Serial.begin(9600);
  dht.begin();

  pinMode(pinoLEDB, OUTPUT);
  pinMode(pinoLEDG, OUTPUT);
  pinMode(pinoLEDR, OUTPUT);
  pinMode(pinoBuzzer, OUTPUT);
  pinMode(pino_microfone, INPUT);

  pinMode(2,INPUT_PULLUP);
  pinMode(13,OUTPUT);
  TaskHandle task1 = {proc1, 500, SIM}; 
  TaskHandle task2 = {proc2, 110, SIM};
  TaskHandle task3 = {proc3, 500, SIM};
  KernelInit();
  KernelAdd(&task1);
  KernelAdd(&task2);
  KernelAdd(&task3);
  KernelLoop();
}
