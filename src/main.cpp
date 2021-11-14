#include <Arduino.h>

typedef union _flag{
    struct bit{
        uint8_t b0: 1;
        uint8_t b1: 1;
        uint8_t b2: 1;
        uint8_t b3: 1;
        uint8_t b4: 1;
        uint8_t b5: 1;
        uint8_t b6: 1;
        uint8_t b7: 1;
    };
    uint8_t byte;
};

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


int timeOut; int f;

uint8_t steps[256];

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

    timeOut = 1;
    f = 1;

    for (uint8_t i = 0; i < 255; i++){
        steps[i] = 127*sin(2*pi*f*i/256)+128;
    }
    
  
}

void loop() {
  
}