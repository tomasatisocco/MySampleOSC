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

// Define the output pins as bits of the R2R and the analog read pin

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

// Define the input ID commands of the protocol

#define   ACK             0x0D
#define   ALIVE           0xF0
#define   BRIDGE          0xA0
#define   TRIFASICBRIDGE  0xA1
#define   PULSESIZEBRIDGE 0xA2
#define   PULSESQNTBRIDGE 0xA3
#define   ONOFF           0xA6
#define   CHANGEBITS      0xA7

// Define the states of the input reader

#define   WAITINGE0   0
#define   WAITING0E   1
#define   WAITINGLB   2
#define   WAITINGHB   3
#define   WAITING3A   4
#define   WAITINGPL   5

//Define the used Flags

#define SCOPEISON flag1.bit.b0
#define SENDDATA flag1.bit.b1

_flag flag1;

void GenerateBridgeBySize(uint8_t anchoDePulso);
void GenerateBridgeByPulses(uint8_t pulsesQuantyty);
void GenerateTrifasicBridge180();
void GenerateTrifasicBridge120();
void GenerateAndReadVoltage(unsigned long waitingTime);
void ChangePinsValue(uint8_t pin, uint8_t value);
void AddDataToTXBuff(unsigned long waitingTime);
void PutHeaderIntx();
void PutByteIntx(uint8_t byte);
void SendTXData();
boolean TXBuffHasData();
void ReadRXBuff();
boolean RXBuffHasData();
void DecodeRXBuff();
void Return(uint8_t id, uint8_t parameter);
void SendACK(uint8_t id, uint8_t parameter, uint8_t hasParameter);

uint8_t checksumTX, checksumRX, stateRead, checksum;
uint8_t indexWriteTX, indexReadTX, indexReadRX, indexWriteRX, indexVoltageWrite, indexVoltageRead, indexSteps;
uint8_t steps[40], rxBuff[256], txBuff[256], pinsValue;
uint16_t lenghtPL, lenghtPLSaved;
uint16_t voltageRead[40],voltageWrite[40];
unsigned long timeout, timeout2;

// Diferent functions to generate the shots secuences needed

void GenerateBridgeBySize(uint8_t anchoDePulso){
  for (uint8_t i = 0; i < 20; i++){
    if ((i >= (10 - anchoDePulso)) && (i <= 10 + anchoDePulso)){
      steps[i] = 0x30;
      steps[i + 20] = 0xC0;
    }
    else {
      steps[i] = 0x00;
      steps[i + 20] = 0x00;
    }
  }
}

void GenerateBridgeByPulses(uint8_t pulsesQuantyty){
  uint8_t counter = 0;
  uint8_t value1 = 0xC0;
  uint8_t value2 = 0x30;
  for (uint8_t i = 0; i < 20; i++){
    if (counter == (20 / (pulsesQuantyty * 2))){
      if (!value1){
        value1 = 0xC0;
        value2 = 0x30;
      } else {
        value1 = value2 = 0;
      }
      counter = 0;
    }
    counter++;
    steps[i] = value1;
    steps[i + 20] = value2;
  }
}

void GenerateTrifasicBridge180(){
  for (uint8_t i = 0; i < 8; i++){
    steps[i] = 0b11100000;
    steps[i + 8] = 0b01110000;
    steps[i + 16] = 0b00111000;
    steps[i + 24] = 0b00011100;
    steps[i + 32] = 0b10001100;
    steps[i + 40] = 0b11000100;
  }
}

void GenerateTrifasicBridge120(){
  for (uint8_t i = 0; i < 8; i++){
    steps[i] = 0b11000000;
    steps[i + 8] = 0b01100000;
    steps[i + 16] = 0b00110000;
    steps[i + 24] = 0b00011000;
    steps[i + 32] = 0b00001100;
    steps[i + 40] = 0b10000100;
  }
}

void ChangePinsValue(uint8_t pin, uint8_t value){
  if (value == 0){
    pinsValue &= ~(0x01 << pin);
  } else {
    pinsValue |= (0x01 << pin);
  }
  for (uint8_t i = 0; i < 48; i++){
    steps[i] = pinsValue;
  }
}

void generateSin(uint8_t f){
  for (uint8_t i = 0; i < 48; i++){
    steps[i] = 127*sin(2*pi*f*i/48)+128;
  }
}

// Functions for chancge the output pins status, read the voltage of R2R 
// and add the data to the TX Buffer with the communication protocol

void GenerateAndReadVoltage(unsigned long waitingTime){
  if ((micros() - timeout) >= waitingTime){
    digitalWrite(BIT0, steps[indexSteps] & 0x01);
    digitalWrite(BIT1, steps[indexSteps] & 0x02);
    digitalWrite(BIT2, steps[indexSteps] & 0x04);
    digitalWrite(BIT3, steps[indexSteps] & 0x08);
    digitalWrite(BIT4, steps[indexSteps] & 0x10);
    digitalWrite(BIT5, steps[indexSteps] & 0x20);
    digitalWrite(BIT6, steps[indexSteps] & 0x40);
    digitalWrite(BIT7, steps[indexSteps] & 0x80);
    if (++indexSteps == 40){
      indexSteps = 0;
    }
    timeout = micros();
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

void SendTXData(){
  if(TXBuffHasData()){
    if(Serial.availableForWrite()){
      Serial.write(txBuff[indexReadTX++]);
    }
  }
}

boolean TXBuffHasData(){
  if (indexReadTX != indexWriteTX){
    return true;
  } else {
    return false;
  }
}

// Functions for Read the Serial inputs, decode it, and
// Answer it

void ReadRXBuff(){
  while (Serial.available()){
    rxBuff[indexWriteRX++] = Serial.read();
  }
  if (RXBuffHasData()){
    DecodeRXBuff();
  }
}

boolean RXBuffHasData(){
  if (indexReadRX != indexWriteRX){
    return true;
  } else {
    return false;
  }
}

void DecodeRXBuff(){
  switch(stateRead){
    case WAITINGE0:
      if( rxBuff[indexReadRX++] == 0xE0 ){    //stateRead = WAITINGLB
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

void Return(uint8_t id, uint8_t parameter){
  uint8_t hasParameter = 0;
  switch (id){
    case ALIVE:
    SendACK(id, parameter, hasParameter);
    break;
    case ONOFF:
      hasParameter = 1;
      if (!parameter){
        SCOPEISON = 0;
      } else {
        SCOPEISON = 1;
      }
      SendACK(id, parameter, hasParameter);
    break;
    case BRIDGE:
      GenerateBridgeBySize(20);
      SendACK(id, parameter, hasParameter);
    break;
    case TRIFASICBRIDGE:
     hasParameter = 1;
      if (parameter){
        GenerateTrifasicBridge120();
      } else {
        GenerateTrifasicBridge180();
      }
      SendACK(id, parameter, hasParameter);
    break;
    case PULSESIZEBRIDGE:
      hasParameter = 1;
      GenerateBridgeBySize(parameter);
      SendACK(id, parameter, hasParameter);
    break;
    case PULSESQNTBRIDGE:
      hasParameter = 1;
      GenerateBridgeByPulses(parameter);
      SendACK(id, parameter, hasParameter);
    break;
    case CHANGEBITS:
      ChangePinsValue(parameter, rxBuff[(indexReadRX - lenghtPLSaved) + 2]);
      SendACK(id,pinsValue,1);
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
  
  GenerateBridgeBySize(10);

  Serial.begin(115200);
}

void loop() {

  ReadRXBuff();

  if (SCOPEISON){
    // Cambia de valor cada tantos microsegundos como se le indica en el input
    GenerateAndReadVoltage(500);
    // Agrega Valores al Buffer de escritura (TX) cada tantos millisegundos ccomo se le indica en el input
    AddDataToTXBuff(20);
  }

  SendTXData();
}