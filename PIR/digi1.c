// DL: Direct Link Interface of the Triple DigiPyro®. A bidirectional single wire interface,
// directly connectable to most μC’s I/O- pins.
// V2.03 10.12.2007 read the Triple Channel DigiPyro® synchronous
// μC: Atmel ATMega8, 6MHz: Port C0 (ADC0, Pin 23) connected to Int0 (Pin 32) external Interrupt.
// DPDL High will cause an Interrupt and so start reading the Triple Channel DigiPyro®
// immediately when ADC has finished
// read 3 x 14 Bit from Triple DigiPyro®, 0=Channel 1, 1= Channel 0, 2=Temperature
#include <mega8.h>
// Definitions for DigiPyro Port C Bit 0
#define DDPDL DDRC.0 // port C bit 0 data direction bit
#define DPDL_OUT PORTC.0 // port C bit 0 output
#define DPDL_IN PINC.0 // port C bit 0 input
#define PORT_IS_INP 0
#define PORT_IS_OUTP 1
int Data[3]; // Data[0]: Channel 1, Data[1]: Channel 0, Data[2]: Temperature (triple: 3 x 14 Bit)
// ****************************************************************************
// External Interrupt 0 service routine
//
// read DigiPyro
//
interrupt [EXT_INT0] void ext_int0_isr(void)
{ // Disable External Interrupt0 until DigiPyro is read.
MCUCR &= 0xFC; // Ext. Interrupt 0 disabled while reading
digipyro();
MCUCR |=0x03; // Ext. Interrupt 0 enabled
}
// ****************************************************************************
// function: digipyro()
//
// do not read asynchronous to the ADC- converter. Read the interface interrupt-driven: High = new data
// this routine should preferrably be used by the interrupt routine only.
// use Data[] for access to the DigiPyro after interrupt
// reads DigiPyro data: Data[0]: Channel 1, Data[1]: Channel 0, Data[2]: Temperature
void digipyro(void)
{ int i, j, k, pir_data;
pir_data = 0;
DDPDL = PORT_IS_INP; // Configure PORT CO as Input for
// DigiPyro Direct Link Interface (DPDL)
while(DPDL_IN == 0); // wait for DPDL = high - max. 1 ms (in regular mode)
for (j=0; j < 60; j++); // wait appr. 30 μsec (tS)
for (k = 0; k < 3; k++) // 3 Channels
{ for (i=0; i < 14; i++) // 14 Bits each
{ DPDL_OUT = 0; // Set DPDL = Low, Low level duration must be > 200 ns (tL)
DDPDL = PORT_IS_OUTP; // Configure PORT C0 DPDL as Output
DPDL_OUT = 0; // Set DPDL = Low, Low level duration must be > 200 ns (tL)
#asm("nop")
DPDL_OUT = 1; // Set DPDL = High, High level duration must be > 200 ns (tH)
#asm("nop")
DDPDL = PORT_IS_INP; // Configure PORT C0 DPDL as Input
#asm("nop")
#asm("nop")
for(j=0; j < 2; j++); // wait appr. 5 μsec to ensure proper low level reading (tbit)
pir_data <<=1;
if (DPDL_IN) pir_data++; // sample bit
}
Data[k] = pir_data;
pir_data = 0;
}
DDPDL = PORT_IS_OUTP; // Configure PORT C0 DPDL as Output
DPDL_OUT = 0; // Set DPDL = Low, Low level duration must be > 200 ns (tL)
DDPDL = PORT_IS_INP; // Configure PORT CO DPDL as Input
return ();
}
void main(void)
{ // Port C initialization
// Func6=In Func5=Out Func4=Out Func3=Out Func2=Out Func1=In Func0=In
// State6=T State5=0 State4=0 State3=0 State2=0 State1=T State0=T
PORTC = 0x00;
DDRC = 0x3C;
// External Interrupt initialization
// INT0: On
// INT0 Mode: Rising Edge, DigiPyro triggers Interrupt to be immediately read after conversion
GICR |= 0x40;
MCUCR &= 0xFC;
GIFR |= 0x40;
SFIOR |= 0x04; // no pullups: PUD=1 (Bit#2)
#asm("sei") // Global enable interrupts
int channel1, channel0, temperature; // DigiPyro data 3 x 14 Bit
channel1 = Data[0];
channel0 = Data[1];
temperature = Data[2];
// ... your code ...
}
