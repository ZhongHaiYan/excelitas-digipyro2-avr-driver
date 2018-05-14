#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "uart.h"

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

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

    DDRD = (1<<3);
    PORTD = (0<<3);
    for(i=0;i < 25;i++){
        nextbit = (regval&regmask)!=0;
        regmask >>= 1;
        //_delay_ms(100);
        PORTD = (0<<3);
        //_delay_ms(100);
        PORTD = (1<<3);
        //_delay_ms(100);
        PORTD = (nextbit<<3);

        _delay_us(100);
    }
    PORTD = (0<<3);
    _delay_us(600);
    puts("writed!");
}


void readlowpowerpyro(void) {
    int i;
    unsigned int uibitmask;
    unsigned long ulbitmask;
    PORTD = (1<<2); // Set DL = High, to force fast uC controlled DL read out
    DDRD = (1<<2); // Configure PORT DL as Output
    _delay_us(150);
    // get first 15bit out-off-range and ADC value
    uibitmask = 0x4000; // Set BitPos
    PIRval = 0;
    for (i=0; i < 15; i++){
        // create low to high transition
        PORTD = (0<<2); // Set DL = Low, duration must be > 200 ns (tL)
        DDRD = (1<<2); // Configure DL as Output
        //_delay_us(1);
        //asm("nop"); // number of nop dependant processor speed (200ns min.)
        PORTD = (1<<2); // Set DL = High, duration must be > 200 ns (tH)
        //_delay_us(1);
        DDRD = (0<<2); // Configure DL as Input
        _delay_us(3); // Wait for stable low signal
        // If DL High set masked bit in PIRVal
        if (PIND & 0x02) PIRval |= uibitmask;
            uibitmask>>=1;
    }
    // get 25bit status and config
    ulbitmask = 0x1000000; // Set BitPos
    statcfg = 0;
    for (i=0; i < 25; i++){
        PORTD = (0<<2); // Set DL = Low, duration must be > 200 ns (tL)
        DDRD = (1<<2); // Configure DL as Output
        //_delay_us(1);
        //asm("nop"); // number of nop dependant processor speed (200ns min.)
        PORTD = (1<<2); // Set DL = High, duration must be > 200 ns (tH)
        //_delay_us(1);
        DDRD = (0<<2); // Configure DL as Input
        _delay_us(3); // Wait for stable low signal, tbd empirically using scope
        // If DL High set masked bit
        if (PIND & 0x02) statcfg |= ulbitmask;
            ulbitmask>>=1;
    }
    PORTD = (0<<2); // Set DL = Low
    DDRD = (1<<2); // Configure DL as Output
    //_delay_us(1);
    DDRD = (0<<2); // Configure DL as Input
    PIRval &= 0x3FFF; // clear unused bit
    if (!(statcfg & 0x60)){
        // ADC source to PIR band pass
        // number in 14bit two's complement
        if(PIRval & 0x2000) PIRval -= 0x4000;
    }
    //puts("readed!!!!");
    //puts("readed 3 : %d",3)
    //printf("hello world %d",3);
    printf("PIRVal: %d\tstatcfg: %ul\n",PIRval,statcfg);
    return;


}
int main(void){
    int i=1;
    uart_init();
    stdout = &uart_output;
    stdin  = &uart_input;
    DDRB = 0x10;
    //DDRD |= (0<<2);
    //PORTD |= (1<<2);
    while(1){

        if(i){
            puts("Writing to register");
            _delay_ms(1000);
            writeregval(0x00C01710);
            i=0;
        }
        //if(!(PIND&0b00000010)){
        //    puts("hareket");
        //}
        puts("Reading from register");
        _delay_ms(1000);
        readlowpowerpyro();
        /*

        if(PINB & 0x20){

            puts("Movement!!!\n");
            DDRB = 0x30;
            PORTB = 0x10;
            puts("Waiting for 2 Sec\n");
            _delay_ms(2000);
            puts("Waiting done\n");
            PORTB = 0x00;
            DDRB = 0x10;
        }
        */

    }
    return 0;
}
