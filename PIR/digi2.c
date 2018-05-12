#include <avr/io.h>
#include <util/delay.h>

#define PIN_IS_INP 0
#define PIN_IS_OUTP 1

#define DSERIAL_IN DDRD.3 // Port Pin Direction Register
#define DLA_DIR DDRD.2 // Direct Link, port D bit 2 data direction bit


#define SERIAL_IN PORTD.3 // Port Pin for LowPower DigiPyro SERIN
#define DLA_OUT PORTD.2 // Direct Link, port D bit 2 output


#define DLA_IN PIND.2 // Direct Link, port D bit 2 input


// SERIN is DDRD.3 out, PORTD.3

int PIRval = 0; // PIR signal
unsigned long statcfg = 0; // status and configuration register

void writeregval(unsigned long regval){
    int i;
    unsigned char nextbit;
    unsigned long regmask = 0x1000000;
    DSERIAL_IN = PORT_IS_OUTP;
    SERIAL_IN = 0;
    for(i=0;i < 25;i++){
        nextbit = (regval&regmask)!=0;
        regmask >>= 1;
        SERIAL_IN = 0;
        SERIAL_IN = 1;
        SERIAL_IN = nextbit;
        _delay_us(100);
    }
    SERIAL_IN = 0;
    _delay_us(600);
}


void readlowpowerpyro(void) {
    int i;
    unsigned int uibitmask;
    unsigned long ulbitmask;
    DLA_OUT = 1; // Set DL = High, to force fast uC controlled DL read out
    DLA_DIR = PIN_IS_OUTP; // Configure PORT DL as Output
    _delay_us(150);
    // get first 15bit out-off-range and ADC value
    uibitmask = 0x4000; // Set BitPos
    PIRval = 0;
    for (i=0; i < 15; i++){
        // create low to high transition
        DLA_OUT = 0; // Set DL = Low, duration must be > 200 ns (tL)
        DLA_DIR = PIN_IS_OUTP; // Configure DL as Output
        #asm("nop") // number of nop dependant processor speed (200ns min.)
        DLA_OUT = 1; // Set DL = High, duration must be > 200 ns (tH)
        DLA_DIR = PIN_IS_INP; // Configure DL as Input
        _delay_us(3); // Wait for stable low signal
        // If DL High set masked bit in PIRVal
        if (DLA_IN) PIRval |= uibitmask;
            uibitmask>>=1;
    }
    // get 25bit status and config
    ulbitmask = 0x1000000; // Set BitPos
    statcfg = 0;
    for (i=0; i < 25; i++){
        // create low to high transition
        DLA_OUT = 0; // Set DL = Low, duration must be > 200 ns (tL)
        DLA_DIR = PIN_IS_OUTP; // Configure DL as Output
        #asm("nop") // number of nop dependant processor speed (200ns min.)
        DLA_OUT = 1; // Set DL = High, duration must be > 200 ns (tH)
        DLA_DIR = PIN_IS_INP; // Configure DL as Input
        _delay_us(3); // Wait for stable low signal, tbd empirically using scope
        // If DL High set masked bit
        if (DLA_IN) statcfg |= ulbitmask;
    ulbitmask>>=1;
    }
    DLA_OUT = 0; // Set DL = Low
    DLA_DIR = PIN_IS_OUTP; // Configure DL as Output
    #asm("nop")
    DLA_DIR = PIN_IS_INP; // Configure DL as Input
    PIRval &= 0x3FFF; // clear unused bit
    if (!(statcfg & 0x60)){
        // ADC source to PIR band pass
        // number in 14bit two's complement
        if(PIRval & 0x2000) PIRval -= 0x4000;
    }
    return;
}
