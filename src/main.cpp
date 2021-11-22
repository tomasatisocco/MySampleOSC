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

#define   ACK             0x0D
#define   ALIVE           0xF0
#define   ONOFF           0xA6
#define   BRIDGE          0xA0
#define   TRIFASICBRIDGE  0xA1
#define   PULSESIZEBRIDGE 0xA2
#define   PULSESQNTBRIDGE 0xA3

#define   WAITINGE0   0
#define   WAITING0E   1
#define   WAITINGLB   2
#define   WAITINGHB   3
#define   WAITING3A   4
#define   WAITINGPL   5

#define SCOPEISON flag1.bit.b0
#define SENDDATA flag1.bit.b1

_flag flag1;

void generateBridge();
void GenerateAndReadVoltage(unsigned long waitingTime);
void ReadRXBuff();
void DecodeRXBuff();
void AddDataToTXBuff(unsigned long waitingTime);
void PutHeaderIntx();
void PutByteIntx(uint8_t byte);
void Return(uint8_t id, uint8_t parameter);
boolean RXBuffHasData();
boolean TXBuffHasData();

uint8_t checksumTX, checksumRX, stateRead, checksum;
uint8_t indexWriteTX, indexReadTX, indexReadRX, indexWriteRX, indexVoltageWrite, indexVoltageRead, indexSteps;
uint8_t steps[42], rxBuff[256], txBuff[256];
uint16_t lenghtPL, lenghtPLSaved;
uint16_t voltageRead[30],voltageWrite[30];
unsigned long timeout, timeout2;

void generateBridge(uint8_t anchoDePulso){
  uint8_t value = 0xC0;
  for (uint8_t i = 0; i < 42; i++){
    if (!(i % (anchoDePulso/10))){
      if (value == 0xC0){
        value = 0x30;
      } else {
        value = 0xC0;
      }
    }
    steps[i] = value;
  }
}

void generatePulsesBridge(uint8_t pulsesQuantyty){
  uint8_t value = 0xC0;
  uint8_t pines;
  for (uint8_t i = 0; i < 42; i++){
    if (!(i % pulsesQuantyty)){
      if (value == 0xC0){
        value = 0x30;
      } else {
        value = 0xC0;
      }
    }
    if (!(i % 2)){
      pines = 0;
    } else {
      pines = value;
    }
    steps[i] = pines;
  }
}

void generateTrifasicBridge180(){
  for (uint8_t i = 0; i < 7; i++){
    steps[i] = 0b11100000;
    steps[i + 7] = 0b01110000;
    steps[i + 14] = 0b00111000;
    steps[i + 21] = 0b00011100;
    steps[i + 28] = 0b10001100;
    steps[i + 34] = 0b11000100;
  }
}

void generateTrifasicBridge120(){
  for (uint8_t i = 0; i < 7; i++){
    steps[i] = 0b11000000;
    steps[i + 7] = 0b01100000;
    steps[i + 14] = 0b00110000;
    steps[i + 21] = 0b00011000;
    steps[i + 28] = 0b00001100;
    steps[i + 34] = 0b10000100;
  }
}

void generateSaw(){
  for (uint8_t i = 0; i < 42; i++){
    steps[i] = i * 6;
  }
}

void generateSin(uint8_t f){
  for (uint8_t i = 0; i < 42; i++){
    steps[i] = 127*sin(2*pi*f*i/42)+128;
  }
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

void GenerateAndReadVoltage(unsigned long waitingTime){
  if ((millis() - timeout) >= waitingTime){
    digitalWrite(BIT0, steps[indexSteps]      & 1);
    digitalWrite(BIT1, steps[indexSteps] >> 1 & 1);
    digitalWrite(BIT2, steps[indexSteps] >> 2 & 1);
    digitalWrite(BIT3, steps[indexSteps] >> 3 & 1);
    digitalWrite(BIT4, steps[indexSteps] >> 4 & 1);
    digitalWrite(BIT5, steps[indexSteps] >> 5 & 1);
    digitalWrite(BIT6, steps[indexSteps] >> 6 & 1);
    digitalWrite(BIT7, steps[indexSteps] >> 7 & 1);
    if (indexSteps++ == 41){
      indexSteps = 0;
    }
    timeout = millis();
    voltageRead[indexVoltageRead++] = analogRead(READER);
  }
}

void AddDataToTXBuff(unsigned long waitingTime){
  if ((millis() - timeout2) > waitingTime){
    PutHeaderIntx();
    PutByteIntx(indexVoltageRead * 2 + 3);
    PutByteIntx(0x00);
    PutByteIntx(0x3A);
    PutByteIntx(0xB0);
    PutByteIntx(indexVoltageRead * 2);
    for (uint8_t i = 0; i < indexVoltageRead; i++){
      PutByteIntx(voltageRead[i] & 0xFF);
      PutByteIntx(voltageRead[i] >> 8);
    }
    PutByteIntx(checksumTX);
    indexVoltageRead = 0;
    timeout2 = millis();
  }
}

void PutHeaderIntx(){
  txBuff[indexWriteTX++] = 0xE0;
  txBuff[indexWriteTX++] = 0x0E;
  checksumTX = 0x0E + 0xE0;
}

void PutByteIntx(uint8_t byte){
  txBuff[indexWriteTX++] = byte;
  checksumTX += byte;
}

boolean RXBuffHasData(){
  if (indexReadRX != indexWriteRX){
    return true;
  } else {
    return false;
  }
}

boolean TXBuffHasData(){
  if (indexReadTX != indexWriteTX){
    return true;
  } else {
    return false;
  }
}

void ReadRXBuff(){
  while (Serial.available()){
    rxBuff[indexWriteRX++] = Serial.read();
  }
  if (RXBuffHasData()){
    DecodeRXBuff();
  }
}

void DecodeRXBuff(){
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
      checksumRX += rxBuff[indexReadRX];
      indexReadRX++;
    break;
    case WAITING3A:
      if (rxBuff[indexReadRX++] == 0x3A){
        checksumRX += 0x3A;
        stateRead = WAITINGPL;
      }
    break;
    case WAITINGPL:
      if (lenghtPL > 1){
        checksumRX += rxBuff[indexReadRX];
      }
      lenghtPL--;
      if (lenghtPL == 0){
        stateRead = WAITINGE0;
        if (checksumRX == rxBuff[indexReadRX]){
          Return(rxBuff[(indexReadRX - lenghtPLSaved)], rxBuff[(indexReadRX - lenghtPLSaved) + 1]);
        }
      }
      indexReadRX++;
    break;
    default:
      stateRead = WAITINGE0;
    break;
  }
}

void SendACK(uint8_t id, uint8_t parameter, uint8_t hasParameter){
  PutHeaderIntx();
  PutByteIntx(hasParameter + 3);
  PutByteIntx(0x00);
  PutByteIntx(0x3A);
  PutByteIntx(id);
  if (hasParameter){
    PutByteIntx(parameter);
  }
  PutByteIntx(ACK);
  PutByteIntx(checksumTX);
}

void Return(uint8_t id, uint8_t parameter){
  uint8_t hasParameter = 0;
  switch (id){
    case ALIVE:
    break;
    case ONOFF:
      hasParameter = 1;
      if (parameter == 0x00){
        SCOPEISON = 0;
      }
      if (parameter == 0x01){
        SCOPEISON = 1;
      }
    break;
    case BRIDGE:
      generateBridge(210);
    break;
    case TRIFASICBRIDGE:
     hasParameter = 1;
      if (parameter){
        generateTrifasicBridge120();
      } else {
        generateTrifasicBridge180();
      }
    break;
    case PULSESIZEBRIDGE:
      hasParameter = 1;
      generateBridge(parameter);
    break;
    case PULSESQNTBRIDGE:
      hasParameter = 1;
      generatePulsesBridge(parameter);
    break;
  }
  SendACK(id, parameter, hasParameter);
}

void SendTXData(){
  if(TXBuffHasData()){
    if(Serial.availableForWrite()){
      Serial.write(txBuff[indexReadTX++]);
    }
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
  
  generateBridge(210);

  Serial.begin(9600);
}

void loop() {

  ReadRXBuff();

  if (SCOPEISON){
    // Cambia de valor cada tantos millisegundos como se le indica en el input
    GenerateAndReadVoltage(10);

    // Agrega Valores al Buffer de escritura (TX) cada tantos millisegundos ccomo se le indica en el input
    AddDataToTXBuff(200);
  }

  SendTXData();
}