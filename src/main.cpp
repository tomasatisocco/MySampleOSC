#include <Arduino.h>

typedef union{
  struct{
    uint8_t b0: 1;
    uint8_t b1: 1;
    uint8_t b2: 1;
    uint8_t b3: 1;
    uint8_t b4: 1;
    uint8_t b5: 1;
    uint8_t b6: 1;
    uint8_t b7: 1;
  }bit;
  uint8_t byte;
}_flag;

#define BIT0 37
#define BIT1 39
#define BIT2 41
#define BIT3 43
#define BIT4 45
#define BIT5 47
#define BIT6 49
#define BIT7 51
#define READER A14

#define pi 3.1415

#define   ALIVE   0xF0

#define   WAITINGE0   0
#define   WAITING0E   1
#define   WAITINGLB   2
#define   WAITINGHB   3
#define   WAITING3A   4
#define   WAITINGPL   5

#define RUN flag1.bit.b0

void generateSteps();

uint8_t i, checksumTX, checksumRX, stateRead, checksum;
uint8_t steps[256];
uint16_t lenghtPL, lenghtPLSaved;
uint16_t voltage;
unsigned long timeout, timeout2;
uint8_t rxBuff[256], txBuff[256], indexWriteTX, indexReadTX, indexReadRX, indexWriteRX;

void generateSteps(uint8_t f){
  for (uint8_t i = 0; i < 255; i++){
    if (i < 128){
      steps[i] = i * 2;
    } else {
      steps[i] = (255 - i) * 2;
    }
  }
}

void generateSaw(){
  for (uint8_t i = 0; i < 255; i++){
    steps[i] = i;
  }
}

void generateSin(uint8_t f){
  for (uint8_t i = 0; i < 255; i++){
    steps[i] = 127*sin(2*pi*f*i/256)+128;
  }
  steps[255] = 128;
}

void PutHeaderIntx(){
  txBuff[indexWriteTX++] = 0xE0;
  txBuff[indexWriteTX++] = 0x0E;
  checksumTX = 0x0E + 0xE0;
}

void GenerateSquare(){
  for (uint8_t i = 0; i < 255; i++){
    if (i < 128){
      steps [i] = 255;
    } else {
      steps[i] = 0;
    }
  }
}

void PutByteIntx(uint8_t byte){
  txBuff[indexWriteTX++] = byte;
  checksumTX += byte;
}

void Return(){
  switch (rxBuff[(indexReadRX - lenghtPLSaved)]){
    case ALIVE:
      PutHeaderIntx();
      PutByteIntx(0x03);
      PutByteIntx(0x00);
      PutByteIntx(0x3A);
      PutByteIntx(0xF0);
      PutByteIntx(0x0D);
      PutByteIntx(checksumTX);
    break;
  }
}

void setup() {                                                                                                                                                                                                                                          //Tomas Tisocco maderfaker

  pinMode(BIT0, OUTPUT);
  pinMode(BIT1, OUTPUT);
  pinMode(BIT2, OUTPUT);
  pinMode(BIT3, OUTPUT);
  pinMode(BIT4, OUTPUT);
  pinMode(BIT5, OUTPUT);
  pinMode(BIT6, OUTPUT);
  pinMode(BIT7, OUTPUT);

  stateRead = WAITINGE0;
  pinMode(10, OUTPUT);
  generateSteps(1);

  Serial.begin(9600);
}

void loop() {
  while (Serial.available()){
    rxBuff[indexWriteRX++] = Serial.read();
  }
  if (indexReadRX != indexWriteRX){
    switch(stateRead){
      case WAITINGE0:
        if( rxBuff[indexReadRX++] == 0xE0 ){
          stateRead = WAITING0E;
        }
      break;
        case WAITING0E:
        if( rxBuff[indexReadRX++] == 0x0E ){
          stateRead = WAITINGLB;
        } else {
          stateRead = WAITINGE0;
        }
      break;
      case WAITINGLB:
        lenghtPL = rxBuff[indexReadRX];
        lenghtPLSaved = (rxBuff[indexReadRX] - 1);
        checksumRX = 0xE0 + 0x0E + rxBuff[indexReadRX];
        stateRead = WAITINGHB;
        indexReadRX++;
      break;
      case WAITINGHB:
        stateRead = WAITING3A;
        lenghtPL = lenghtPL + 256 * rxBuff[indexReadRX];
        checksumRX = checksumRX + rxBuff[indexReadRX];
        indexReadRX++;
      break;
      case WAITING3A:
        if (rxBuff[indexReadRX++] == 0x3A){
          checksumRX = checksumRX + 0x3A;
          stateRead = WAITINGPL;
        }
      break;
      case WAITINGPL:
        if (lenghtPL > 1){
          checksumRX = checksumRX + rxBuff[indexReadRX];
        }
        lenghtPL--;
        if (lenghtPL == 0){
          stateRead = WAITINGE0;
          if (checksumRX == rxBuff[indexReadRX]){
            Return();
          }
        }
        indexReadRX++;
      break;
      default:
        stateRead = WAITINGE0;
    }
  }
  if(indexWriteTX != indexReadTX){
    if(Serial.availableForWrite()){
      Serial.write(txBuff[indexReadTX++]);
    }
  }
  if ((millis() - timeout) >= 0){
    digitalWrite(BIT0, steps[i]      & 1);
    digitalWrite(BIT1, steps[i] >> 1 & 1);
    digitalWrite(BIT2, steps[i] >> 2 & 1);
    digitalWrite(BIT3, steps[i] >> 3 & 1);
    digitalWrite(BIT4, steps[i] >> 4 & 1);
    digitalWrite(BIT5, steps[i] >> 5 & 1);
    digitalWrite(BIT6, steps[i] >> 6 & 1);
    digitalWrite(BIT7, steps[i] >> 7 & 1);
    i++;
    timeout = millis();
  }

  if ((millis() - timeout2) >= 0){
    voltage = analogRead(READER);
    checksum = 0x0E + 0xE0 + 0x05 + 0x3A + 0xB0 + 0x02 + (voltage & 0x00FF) + (voltage >> 8);
    Serial.write(0xE0);
    Serial.write(0x0E);
    Serial.write(0x05);
    Serial.write(0x00);
    Serial.write(0x3A);
    Serial.write(0xB0);
    Serial.write(0x02);// enviar cantidad de bytes no cantidad de muestras.
    Serial.write(voltage & 0x00FF);
    Serial.write(voltage >> 8);
    Serial.write(checksum);
    //Serial.println(voltage);
    timeout2 = millis();
  }
}