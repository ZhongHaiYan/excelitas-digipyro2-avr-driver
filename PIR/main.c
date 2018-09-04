#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "uart.h"

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#define DLREG DDRB
#define SINREG DDRD
#define DLPORT PORTB
#define SINPORT PORTD
#define DLIN PINB
#define SININ PIND
#define DLPIN 5
#define SINPIN 2


/*
SERIN to D2 pin,
D/L to D3 pin for reading mode, B5 for interrupt mode.
LED to B4 pin in interrupt mode.
*/

int PIRval = 0; // PIR signal
unsigned long statcfg = 0; // status and configuration register


void writeregval(unsigned long regval){
    int i;
    unsigned char nextbit;
    unsigned long regmask = 0x1000000;
    SINREG = (1<<SINPIN); //Set Serial In Port Output
    SINPORT = (0<<SINPIN); // Set Serial In Pin LOW
    for(i=0;i < 25;i++){
        nextbit = (regval&regmask)!=0; //Set bit value to LSB register value
        regmask >>= 1; //shift mask

        SINPORT = (0<<SINPIN); //Set pin LOW

        SINPORT = (1<<SINPIN); //Set pin HIGH

        SINPORT = (nextbit<<SINPIN); //Set pin to bit value

        _delay_us(100);
    }
    SINPORT = (0<<SINPIN); //Set pin LOW to end the operation
    _delay_us(600);
    puts("writed!");
}

void readPIR(void){ // Valid function to read ADC PIR value, to use this function sensor should be configured for continuous reading
    int i;
    int a = 0;
    unsigned int uibitmask;
    unsigned long ulbitmask;

    DLPORT = (1<<DLPIN); // Set DL = High, to force fast uC controlled DL read out
    DLREG = (1<<DLPIN); // Configure PORT DL as Output
    _delay_us(110);
    // get first 15bit out-off-range and ADC value
    uibitmask = 0x4000; // Set BitPos
    PIRval = 0;
    for (i=0; i < 15; i++){
        // create low to high transition
        DLPORT = (0<<DLPIN); // Set DL = Low, duration must be > 200 ns (tL)
        DLREG = (1<<DLPIN); // Configure DL as Output
        //_delay_us(1);
        a=a;
        //asm("nop"); // number of nop dependant processor speed (200ns min.)
        DLPORT = (1<<DLPIN); // Set DL = High, duration must be > 200 ns (tH)
        //_delay_us(1);
        a=a;
        DLREG = (0<<DLPIN); // Configure DL as Input
        _delay_us(7); // Wait for stable low signal
        // If DL High set masked bit in PIRVal

        if (PINB & (1<<DLPIN)) PIRval |= uibitmask;
            uibitmask>>=1;
    }
    DLPORT = (0<<DLPIN); // Set DL = Low
    DLREG = (1<<DLPIN); // Configure DL as Output
    _delay_us(200);
    DLREG = (0<<DLPIN); // Configure DL as Input
    PIRval &= 0x3FFF; // clear unused bit
    printf("%d\n",PIRval);
    return;
}

void read(void){
    int i;
    int a = 0;
    unsigned int uibitmask;
    unsigned long ulbitmask;

    ulbitmask = 0x1000000; // Set BitPos
    statcfg = 0;

    uibitmask = 0x4000; // Set BitPos
    PIRval = 0;

    DLPORT = (1<<DLPIN); // Set DL = High, to force fast uC controlled DL read out
    DLREG = (1<<DLPIN); // Configure PORT DL as Output
    _delay_us(110);
    // get first 15bit out-off-range and ADC value

    for (i=0; i < 15; i++){
        // create low to high transition
        DLPORT = (0<<DLPIN); // Set DL = Low, duration must be > 200 ns (tL)
        DLREG = (1<<DLPIN); // Configure DL as Output
        //_delay_us(1);
        a=a;
        //asm("nop"); // number of nop dependant processor speed (200ns min.)
        DLPORT = (1<<DLPIN); // Set DL = High, duration must be > 200 ns (tH)
        //_delay_us(1);
        a=a;
        DLREG = (0<<DLPIN); // Configure DL as Input
        _delay_us(4); // Wait for stable low signal
        // If DL High set masked bit in PIRVal

        if (DLIN & 0x20) PIRval |= uibitmask;
            uibitmask>>=1;
    }
    // get 25bit status and config

    for (i=0; i < 25; i++){
        DLPORT = (0<<DLPIN); // Set DL = Low, duration must be > 200 ns (tL)
        DLREG = (1<<DLPIN); // Configure DL as Output
        //_delay_us(1);
        a=a;
        //asm("nop"); // number of nop dependant processor speed (200ns min.)
        DLPORT = (1<<DLPIN); // Set DL = High, duration must be > 200 ns (tH)
        //_delay_us(1);
        a=a;
        DLREG = (0<<DLPIN); // Configure DL as Input
        _delay_us(4); // Wait for stable low signal, tbd empirically using scope
        // If DL High set masked bit

        if (DLIN & 0x20) statcfg |= ulbitmask;
            ulbitmask>>=1;
    }
    DLPORT = (0<<DLPIN); // Set DL = Low
    DLREG = (1<<DLPIN); // Configure DL as Output
    //_delay_us(1);
    a=a;
    DLREG = (0<<DLPIN); // Configure DL as Input
    PIRval &= 0x3FFF; // clear unused bit

    if (!(statcfg & 0x60)){
        // ADC source to PIR band pass
        // number in 14bit two's complement
        if(PIRval & 0x2000) PIRval -= 0x4000;
    }

    printf("PIRVal: %d\tstatcfg: %ul\n",PIRval,statcfg);
    return;

}
int main(void){
    int i=1;
    uart_init();
    stdout = &uart_output;
    stdin  = &uart_input;
    //DDRB = 0x10;
    //DDRD |= (0<<2);
    //PORTD |= (1<<2);
    while(1){

        if(i){
            puts("Writing to register");
            _delay_ms(1000);
            //writeregval(0x00000030); //For continuous reading
            writeregval(0x00304D10); //For interrupt mode
            i=0;
        }

        // For continuous reading
        //puts("Reading from register");
        //_delay_ms(5);
        //readPIR();
        // END

        // Interrupt Mode
        if(PINB & 0x20){ //checks for changes on B5 pin, interrupt can be used

            puts("Movement!!!\n");
            DDRB = 0x30; //set B5 and B4 to output
            PORTB = 0x10; // set B5 to LOW, this necessary to terminate sensor's internal interrupt, and B4 to HIGH for LED indicator
            puts("Waiting for 2 Sec\n"); // to prevent same movement alert
            _delay_ms(2000);
            puts("Waiting done\n");
            PORTB = 0x00; //set LED to LOW
            DDRB = 0x10; // set B5 to input to catch movement
        }
        //END



    }
    return 0;
}
