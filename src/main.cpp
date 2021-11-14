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

#define RUN flag1.bit.b0

void generateSteps();

int timeOut;
uint8_t i;
uint8_t steps[256];
float voltage;

void generateSteps(uint8_t f){
    for (uint8_t i = 0; i < 255; i++){
        steps[i] = 127*sin(2*pi*f*i/256)+128;
    }
}

void setup() {

    pinMode(BIT0, OUTPUT);
    pinMode(BIT1, OUTPUT);
    pinMode(BIT2, OUTPUT);
    pinMode(BIT3, OUTPUT);
    pinMode(BIT4, OUTPUT);
    pinMode(BIT5, OUTPUT);
    pinMode(BIT6, OUTPUT);
    pinMode(BIT7, OUTPUT);

    pinMode(READER, INPUT);

    generateSteps(1);

    Serial.begin(9600);

    timeOut = 1;
}

void loop() {
    digitalWrite(BIT0, steps[i]      & 1);
    digitalWrite(BIT1, steps[i] >> 1 & 1);
    digitalWrite(BIT2, steps[i] >> 2 & 1);
    digitalWrite(BIT3, steps[i] >> 3 & 1);
    digitalWrite(BIT4, steps[i] >> 4 & 1);
    digitalWrite(BIT5, steps[i] >> 5 & 1);
    digitalWrite(BIT6, steps[i] >> 6 & 1);
    digitalWrite(BIT7, steps[i] >> 7 & 1);

    voltage = analogRead(READER) / 1024.0 * 5.0;
    Serial.println(voltage);
    i++;
    delay(100);
}