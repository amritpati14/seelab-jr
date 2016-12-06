/*
 * File:   functions.c
 * Author: jithin
 * Comments : All functions for hardware access go here. 
 * Created on 2 October, 2016, 9:46 AM
 */


#include "xc.h"
#include"functions.h"

void __attribute__((interrupt, no_auto_psv)) _AD1Interrupt(void) {
    _AD1IF = 0;
    if (conversion_done) {
        return;
    }
    LEDPIN=1;
    if(TRIGGERED){
        *(buff0++) = (ADC1BUF0 );
        if (ADC_CHANNELS >= 1){
            *(buff1++) = (ADC1BUF1 );
            if (ADC_CHANNELS >= 2) {
                *buff2++ = (ADC1BUF2 );
                if (ADC_CHANNELS >= 3)*buff3++ = (ADC1BUF3 );
            }
        }
        samples++;
        LEDPIN=0;
        if(samples==samples_to_fetch){  //finished acquisition
            _AD1IF = 0;   _AD1IE = 0;   //disable any further interrupts until required
            conversion_done = 1;
            LEDPIN=1;
            }
        }
    else{
        if(TRIGGER_CHANNEL&1)adval=ADC1BUF0;
        else if(TRIGGER_CHANNEL&2)adval=ADC1BUF1;
        else if(TRIGGER_CHANNEL&4)adval=ADC1BUF2;
        else if(TRIGGER_CHANNEL&8)adval=ADC1BUF3;

        //-------If the trigger hasn't timed out yet ... ----------
        if(TRIGGER_WAITING<TRIGGER_TIMEOUT){
            TRIGGER_WAITING+=(ADC_DELAY>>TRIGGER_PRESCALER);
            if(!TRIGGER_READY && adval>TRIGGER_LEVEL+10)TRIGGER_READY=1;
            else if(adval<=TRIGGER_LEVEL && TRIGGER_READY){TRIGGERED=1;}
            }
       //-------If the trigger has timed out, then proceed to data acquisition ----------
        else {TRIGGERED=1;}
    }
    
}

void __attribute__((__interrupt__, no_auto_psv)) _DMA0Interrupt(void)
{
IC1CON2bits.TRIGSTAT = 0;IC1CON1bits.ICM=0;DMA0CONbits.CHEN = 0;
_DMA0IF = 0;_DMA0IE = 0; // Clear the DMA0 Interrupt Flag
if(DMA_MODE == DMA_LA_TWO_CHAN || DMA_MODE == DMA_LA_ONE_CHAN){IC2CON1bits.ICM=0; DMA1CONbits.CHEN = 0;_DMA1IF = 0;_DMA1IE = 0;}//disable DMA1 too
}
void __attribute__((__interrupt__, no_auto_psv)) _DMA1Interrupt(void)
{
IC2CON2bits.TRIGSTAT = 0;IC2CON1bits.ICM=0; DMA1CONbits.CHEN = 0;
_DMA1IF = 0;_DMA1IE = 0; // Clear the DMA0 Interrupt Flag
}
void __attribute__((__interrupt__, no_auto_psv)) _DMA2Interrupt(void){
IC3CON2bits.TRIGSTAT = 0;IC3CON1bits.ICM=0;
_DMA2IF = 0; // Clear the DMA0 Interrupt Flag
}

void __attribute__((__interrupt__, no_auto_psv)) _DMA3Interrupt(void)
{
IC4CON2bits.TRIGSTAT = 0;IC4CON1bits.ICM=0;
_DMA3IF = 0; // Clear the DMA0 Interrupt Flag
}

void __attribute__((__interrupt__, no_auto_psv)) _INT2Interrupt(void)
{
    IC1CON1bits.ICM=LAM1;IC2CON1bits.ICM=LAM2;IC3CON1bits.ICM=LAM3;IC4CON1bits.ICM=LAM4;
    INITIAL_DIGITAL_STATES =((PORTB>>10)&0xF)|(_C4OUT<<4); //INITIAL_DIGITAL_STATES =(PORTB>>10)&0xF;
    IC1CON2bits.TRIGSTAT = 1;IC3CON2bits.TRIGSTAT = 1;
    IC2CON2bits.TRIGSTAT = 1;IC4CON2bits.TRIGSTAT = 1;
    INITIAL_DIGITAL_STATES_ERR=((PORTB>>10)&0xF)|(_C4OUT<<4); // =(PORTB>>10)&0xF;

    _INT2IF=0;_INT2IE=0;

}



void initADCCTMU(void){
    _AD1IF = 0; _AD1IE = 0;                                             //disable ADC interrupts
    disableADCDMA();DisableComparator();
    AD1CON1bits.ADON = 0;                                               //turn off ADC
    AD1CON2 = 0;
    AD1CON4 = 0x0000;
    AD1CSSH = 0x0000;
    AD1CSSL = 0x0000;
    AD1CON1bits.AD12B = 1;
    /* Assign MUXA inputs for block read */
    AD1CHS0bits.CH0SA = CHOSA;
    AD1CON3bits.ADRC = 0; //do not use internal clock
    AD1CON1bits.SSRCG = 0;
    AD1CON1bits.SSRC = 0b000; //Clearing SAMP bit stops sampling and triggers conversion
    AD1CON1bits.SIMSAM = 0; //simultaneous sampling.
    AD1CON1bits.ASAM = 0; //no auto sampling
    AD1CON3bits.SAMC = 0x10; // Sample for (x+1)*Tad before triggering conversion
    AD1CON2bits.SMPI = 0;
    AD1CON3bits.ADCS = 0xA; // Conversion clock x*Tp
    
}

void set_cap_voltage(BYTE v,unsigned int time){
        _TRISA9=0;
        if(v)_LATA9=1;
        else _LATA9=0; 
        
        //_TRISB3=0;_ANSB3=0;_LATB3=0;
        Delay_us(time);
        _LATA9=0;_TRISA9=1;  Nop();           //Return A9 to high impedance
        //_TRISB3=1;_ANSB3=1;_LATB3=1;

}
void disableCTMUSource(void){
    CTMUCON1bits.CTMUEN = 0;CTMUCON2bits.EDG1STAT = 0;CTMUCON2bits.EDG2STAT = 0; CTMUCON1bits.TGEN = 0; // Stop current source and disable CTMU
}
unsigned int get_cc_capacitance(BYTE current_range,BYTE trimval,unsigned int ChargeTime){
    unsigned int sum=0;
    setADCMode(ADC_CTMU,5,0);   // 5 is the CAP channel
    CTMUCON1bits.TGEN = 1;
    CTMUICONbits.ITRIM = trimval;
    CTMUICONbits.IRNG = current_range;    // 01->Base Range .53uA, 10->base*10, 11->base*100, 00->base*1000
    T5CONbits.TON = 0;  T5CONbits.TGATE = 0;  T5CONbits.TCKPS = 2;   PR5 = ChargeTime;
    set_cap_voltage(0,50000);
    AD1CON1bits.ADON=1;Delay_us(20);
    AD1CON1bits.SAMP = 1;     //start sampling
    CTMUCON1bits.CTMUEN = 1;Delay_us(1000); //CTMU settling delay
    CTMUCON1bits.IDISSEN = 1; //Ground the charge PUMP
    Delay_us(1500);           //Grounding the ADC S&H
    
    TMR5 = 0x0000;_T5IF=0;_T5IE=0;
    CTMUCON1bits.IDISSEN = 0; //stop draining the circuit
    CTMUCON2bits.EDG1STAT = 1; //start charging    
    T5CONbits.TON = 1;
    while(!_T5IF); _T5IF=0;_AD1IF = 0; //wait for charge
    AD1CON1bits.SAMP = 0;       // Begin analog-to-digital conversion
    CTMUCON2bits.EDG1STAT = 0;  // Stop charging circuit
    
    while(!_AD1IF); _AD1IF = 0;
    while (!AD1CON1bits.DONE);
    sum=(ADC1BUF0)&0xFFF;
    disableCTMUSource();
    AD1CON1bits.ADON=0;
    return sum;

}

unsigned int get_cap_range(unsigned int time){
    unsigned int sum=0;
    set_cap_voltage(0,50000);
    set_cap_voltage(0,50000);
    setADCMode(ADC_12BIT_AVERAGING,5,0);
    T5CONbits.TON = 0; TMR5 = 0x0000; _T5IF=0; PR5 = time;T5CONbits.TCKPS = 2;T5CONbits.TON = 1;
    _TRISA9=0;_LATA9=1; //START CHARGING VIA A9,10K resistor
    while(!_T5IF); _T5IF=0;
    _TRISA9=1;_LATA9=0; //STOP CHARGING.
    sum=get_voltage_summed(5); // This function will turn on the ADC, and turn it off when finished.
    return sum;
}

unsigned int get_ctmu_voltage(BYTE channel,BYTE range,BYTE tgen){
    unsigned int temp=0;
    CTMUCON1bits.TGEN = tgen;                       //(channel==5)?1:0;
    CTMUICONbits.ITRIM = 0;
    CTMUICONbits.IRNG = range;                      // 01->Base Range .53uA, 10->base*10, 11->base*100, 00->base*1000
    CTMUCON2bits.EDG1STAT = 0;
    CTMUCON1bits.CTTRIG = 0; //do not trigger the ADC
    if(channel!=30)CTMUCON2bits.EDG2STAT = 0; 
    else CTMUCON2bits.EDG2STAT = 1;     //30 -> internal temperature.
    CTMUCON1bits.CTMUEN = 1;Delay_us(1000);
    
    CTMUCON1bits.IDISSEN = 1; //Ground the charge PUMP
    Delay_us(1500);  //Grounding the ADC S&H
    CTMUCON1bits.IDISSEN = 0; //stop draining the circuit
    
    CTMUCON1bits.CTMUSIDL = 0; //0=enable operation in idle mode
    CTMUCON2bits.EDG1STAT = 1;  // Start current source
    temp = get_voltage_summed(channel);
    disableCTMUSource();
    return temp;

}

void init_IC_for_frequency(BYTE capture_pin,BYTE capture_mode,BYTE captures_per_interrupt){

    RPINR7bits.IC1R = DIN_REMAPS[capture_pin];
    
    IC1CON1bits.ICM=0;    IC2CON1bits.ICM=0;  //disable the module
    IC1CON1bits.ICOV=0;   IC2CON1bits.ICOV=0; //reset overflow flag

    IC1CON1bits.ICTSEL = 0b111;
    IC2CON1bits.ICTSEL = 0b111;
    IC1CON1bits.ICI = captures_per_interrupt-1; 
    IC2CON1bits.ICI = captures_per_interrupt-1;
    IC1CON2bits.IC32 = 1;
    IC2CON2bits.IC32 = 1;
    IC1CON2bits.TRIGSTAT = 0;
    IC2CON2bits.TRIGSTAT = 0;
    IC1CON2bits.SYNCSEL =0; 
    IC2CON2bits.SYNCSEL =0; 
    IC1CON2bits.ICTRIG = 1;
    IC2CON2bits.ICTRIG = 1;
    INITIAL_DIGITAL_STATES =((PORTB>>10)&0xF)|(_C4OUT<<4);
    IC1CON1bits.ICM=capture_mode;
    IC2CON1bits.ICM=capture_mode;

    IC1CON2bits.TRIGSTAT = 1;    IC2CON2bits.TRIGSTAT = 1;

}

void Interval(BYTE capture_pin1,BYTE capture_pin2,BYTE pin1_edge,BYTE pin2_edge){

    if(capture_pin1==4) EnableComparator();
    RPINR7bits.IC1R = DIN_REMAPS[capture_pin1];
    if(capture_pin2==4) EnableComparator();
    RPINR8bits.IC3R = DIN_REMAPS[capture_pin2];


    IC1CON1bits.ICM=0;IC3CON1bits.ICM=0; 
    IC2CON1bits.ICM=0;IC4CON1bits.ICM=0; 
    IC1CON1bits.ICOV=0;IC3CON1bits.ICOV=0;
    IC2CON1bits.ICOV=0;IC4CON1bits.ICOV=0;

    IC1CON1bits.ICTSEL = 0b111;IC3CON1bits.ICTSEL = 0b111;
    IC2CON1bits.ICTSEL = 0b111;IC4CON1bits.ICTSEL = 0b111;
    IC1CON1bits.ICI = 0;IC3CON1bits.ICI = 0;
    IC1CON2bits.IC32 = 1;IC3CON2bits.IC32 = 1;
    IC2CON2bits.IC32 = 1;IC4CON2bits.IC32 = 1;
    IC1CON2bits.ICTRIG = 0;IC1CON2bits.SYNCSEL =0;
    IC2CON2bits.ICTRIG = 0;IC2CON2bits.SYNCSEL =0;

    IC3CON2bits.ICTRIG = 0;IC3CON2bits.SYNCSEL =0;
    IC4CON2bits.ICTRIG = 0;IC4CON2bits.SYNCSEL = 0;
    IC1CON2bits.TRIGSTAT = 0;IC2CON2bits.TRIGSTAT = 0;
    IC3CON2bits.TRIGSTAT = 0;IC4CON2bits.TRIGSTAT = 0;

    _IC1IF=0;_IC3IF=0;
    IC1CON1bits.ICM=pin1_edge;IC2CON1bits.ICM=pin1_edge;
    IC3CON1bits.ICM=pin2_edge;IC4CON1bits.ICM=pin2_edge;
    IC1CON2bits.TRIGSTAT = 1;    IC2CON2bits.TRIGSTAT = 1;
    IC3CON2bits.TRIGSTAT = 1;    IC4CON2bits.TRIGSTAT = 1;

}

void TimingMeasurements(BYTE capture_pin1,BYTE capture_pin2,BYTE pin1_edge,BYTE pin2_edge,BYTE interrupts1,BYTE interrupts2){
    _IC1IF=0;_IC3IF=0;
    if(capture_pin1==4) EnableComparator();
    RPINR7bits.IC1R = DIN_REMAPS[capture_pin1];
    if(capture_pin2==4) EnableComparator();
    RPINR8bits.IC3R = DIN_REMAPS[capture_pin2];
    
    IC1CON1bits.ICM=0;IC3CON1bits.ICM=0; 
    IC2CON1bits.ICM=0;IC4CON1bits.ICM=0; 
    IC1CON1bits.ICOV=0;IC3CON1bits.ICOV=0;
    IC2CON1bits.ICOV=0;IC4CON1bits.ICOV=0;

    T2CONbits.TON = 0;
    T2CONbits.T32 = 0; T2CONbits.TCS = 0;
    T2CONbits.TCKPS = 0;
    TMR2 = 0x0000;

    IC1CON1bits.ICTSEL = 0b1;IC3CON1bits.ICTSEL = 0b1;
    IC2CON1bits.ICTSEL = 0b1;IC4CON1bits.ICTSEL = 0b1;
    IC1CON1bits.ICI = interrupts1-1;IC3CON1bits.ICI = interrupts2-1;
    IC1CON2bits.IC32 = 1;IC3CON2bits.IC32 = 1;
    IC2CON2bits.IC32 = 1;IC4CON2bits.IC32 = 1;
    IC1CON2bits.SYNCSEL =0;IC3CON2bits.SYNCSEL =0;
    IC2CON2bits.SYNCSEL =0;IC4CON2bits.SYNCSEL =0;
    IC1CON2bits.ICTRIG = 0;IC3CON2bits.ICTRIG = 0;
    IC2CON2bits.ICTRIG = 0;IC4CON2bits.ICTRIG = 0;
    INITIAL_DIGITAL_STATES =((PORTB>>10)&0xF)|(_C4OUT<<4);

    IC1CON1bits.ICM=pin1_edge;IC3CON1bits.ICM=pin2_edge;
    IC2CON1bits.ICM=pin1_edge;IC4CON1bits.ICM=pin2_edge;

    IC1CON2bits.TRIGSTAT = 1;    IC2CON2bits.TRIGSTAT = 1;
    IC3CON2bits.TRIGSTAT = 1;    IC4CON2bits.TRIGSTAT = 1;

    T2CONbits.TON = 1; // Start the timer

}

void EnableComparator(){
        /*----setup comparator---*/
    CVRCONbits.VREFSEL = 0;				// Voltage reference is generated by resistor network
    CVRCONbits.CVREN = 1;				// Enable comparator reference source
    CVRCONbits.CVRR = 0;				// Step size is CVRSRC/32
    CVRCONbits.CVR = 7;		                        // CVREFIN = (1/4)*(3.3) + (7/32)*(3.3) = 1.54V

    CM4CONbits.CCH = 0;					// VIN - input of comparator connects to C4IN1-
    CM4CONbits.CREF = 1;				// VIN+ input connected to CVRefin voltage source
    CM4CONbits.EVPOL = 1;				// Trigger/Event/Interrupt generated on high to low
    CM4CONbits.CPOL = 0;				// Comparator output is not inverted
    CM4CONbits.CEVT=0;
    CM4CONbits.COE = 1;					// Comparator output is present on CxOUT pin
    CM4FLTRbits.CFSEL = 0;				// Choose CPU instruction clock (TCY)
    CM4FLTRbits.CFDIV = 4;	                        // Choose comparator filter clock
    CM4FLTRbits.CFLTREN = 1;    			// Digital filter enabled
    CM4MSKSRCbits.SELSRCA = 1;          		// PWM1H1 is the source for Mask A input
    CM4MSKCONbits.HLMS = 1;				// Mask input will prevent comparator output
    CM4MSKCONbits.OAEN = 1;				// OR Gate A input enabled
    CM4CONbits.CON = 1;					// Comparator is enabled
    PMD3bits.CMPMD = 0;                                 // Enable Comparator
}

void DisableComparator(){
    CM4CONbits.CON = 0; PMD3bits.CMPMD = 1;                 
}

void __attribute__ ((__interrupt__, no_auto_psv)) _IC4Interrupt(void){
INITIAL_DIGITAL_STATES =((PORTB>>10)&0xF)|(_C4OUT<<4); //INITIAL_DIGITAL_STATES =(PORTB>>10)&0xF;
IC4CON1bits.ICM=0;     //Disable IC4 interrupt
_IC4IF=0;_IC4IE=0;  //disable input capture interrupt
INITIAL_DIGITAL_STATES_ERR =((PORTB>>10)&0xF)|(_C4OUT<<4); //=(PORTB>>10)&0xF;
}


void disable_input_capture(){
    IC1CON2bits.TRIGSTAT = 0;  IC2CON2bits.TRIGSTAT = 0;IC3CON2bits.TRIGSTAT = 0;  IC4CON2bits.TRIGSTAT = 0;
    IC1CON1bits.ICM=0;  IC2CON1bits.ICM=0;IC3CON1bits.ICM=0;  IC4CON1bits.ICM=0; //disable the module
    IC1CON1bits.ICOV=0; IC2CON1bits.ICOV=0;IC3CON1bits.ICOV=0; IC4CON1bits.ICOV=0; //reset overflow flag
    /*
    _DMA0IF = 0; // Clear the DMA interrupt flag bit
    _DMA0IE = 0; // Disable DMA interrupt enable bit
    DMA0CONbits.CHEN = 0;
    DMA1CONbits.CHEN = 0;
    DMA2CONbits.CHEN = 0;
    DMA3CONbits.CHEN = 0;
     * */
}

void alternate_get_high_frequency(BYTE channel,BYTE scale){ //Measure freq using only input captures. Timer 3 not available

    RPINR7bits.IC1R = 0;    RPINR7bits.IC2R = 0;
    T2CONbits.TON=0;
    T2CONbits.T32=0;
    T2CONbits.TCS = 1; // Select External clock
    T2CONbits.TCKPS = scale; // Select Prescaler

    if(channel==4) EnableComparator();
    RPINR3bits.T2CKR = DIN_REMAPS[channel];


    IC1CON1bits.ICM=0;  IC2CON1bits.ICM=0; //disable the module
    IC1CON1bits.ICTSEL = 1;  IC2CON1bits.ICTSEL = 1; //timer 2
    IC1CON1bits.ICI = 1;       IC2CON1bits.ICI = 1;
    IC1CON1bits.ICOV=0; IC2CON1bits.ICOV=0; //reset overflow flag
    IC1CON2bits.IC32 = 1;      IC2CON2bits.IC32 = 1;
    IC1CON2bits.TRIGSTAT = 0;  IC2CON2bits.TRIGSTAT = 0;
    IC1CON2bits.ICTRIG = 1;    IC2CON2bits.ICTRIG = 1;
    IC1CON2bits.SYNCSEL =0;    IC2CON2bits.SYNCSEL = 0;  //no sync

    
    T5CONbits.TON = 0; // Stop any 16/32-bit Timer5 operation
    T5CONbits.TCKPS = 3; //1:256 , 1.0/8 MHz
    PR5=25000;   //100mS sampling
    TMR5=0x0000;

    _T5IP = 0x01;// Set Timer5 Interrupt Priority Level
    _T5IF = 0; // Clear Timer5 Interrupt Flag
    TMR2 = 0x0000;
    T2CONbits.TON=1;

    IC2CON1bits.ICM=EVERY_SIXTEENTH_RISING_EDGE;  IC1CON1bits.ICM=EVERY_SIXTEENTH_RISING_EDGE;
    T5CONbits.TON = 1; // Start 16-bit Timer
    IC2CON2bits.TRIGSTAT = 1;IC1CON2bits.TRIGSTAT = 1;  
}

void get_high_frequency(BYTE channel,BYTE scale){ //T2CK is tied to ID1. Using timer 3/2

    if(channel==4) EnableComparator();
    RPINR3bits.T2CKR = DIN_REMAPS[channel];

    T3CONbits.TON = 0; // Stop any 16-bit Timer3 operation
    T2CONbits.TON = 0; // Stop any 16/32-bit Timer2 operation
    T5CONbits.TON = 0; // Stop any 16/32-bit Timer5 operation

    T2CONbits.T32 = 1;      // 32 bit mode T2 and T3
    T2CONbits.TCS = 1; // Select External clock
    T2CONbits.TCKPS = scale; // Select Prescaler
    IFS0bits.T3IF = 0;
    T5CONbits.TCKPS = 3; //1:256 , 1.0/8 MHz
    PR5=25000;   //100mS sampling
    TMR3HLD=0;
    TMR3 = 0x0000; // Clear 32-bit Timer (msw)
    TMR2 = 0x0000; // Clear 32-bit Timer (lsw)
    TMR5=0x0000;


    _T5IP = 0x01; // Set Timer5 Interrupt Priority Level
    _T5IF = 0; // Clear Timer5 Interrupt Flag
    //_T5IE = 1; // Enable Timer5 interrupt
    T2CONbits.TON = 1; // Start 32-bit Timer
    T5CONbits.TON = 1; // Start 16-bit Timer

}


void initSPI(){

    SPI1STAT = 0; 
    IFS0bits.SPI1IF = 0;
    IEC0bits.SPI1IE = 0;

    SPI1CON1bits.PPRE = 1;
    SPI1CON1bits.SPRE = 7;
    SPI1CON1bits.DISSCK = 0;

    SPI1CON1bits.DISSDO = 0;
    SPI1CON1bits.SSEN = 0; // CS not used
    SPI1CON1bits.MODE16 = 1; //0=8,1=16 bits wide data
    SPI1CON1bits.CKE = 1; //  !=CPHA
    SPI1CON1bits.CKP = 0; //Clock idle state
    SPI1CON1bits.MSTEN = 1;

    SPI1CON1bits.SMP = 1;
    SPI1STATbits.SPIEN = 1; //enable SPI1
}

unsigned int spi_write16(unsigned int value) {
    SPI1STATbits.SPIROV = 0;
    SPI1BUF = value;
    while (SPI1STATbits.SPITBF); // wait for the data to be sent out
    while (!SPI1STATbits.SPIRBF); // wait for dummy byte to clock in
    return SPI1BUF; // dummy read of the SPI1BUF register to clear the SPIRBF flag
}

void set_CS(BYTE channel,BYTE status){

    if (channel == CSNUM_A1)CSCH1 = status;
    else if (channel == CSNUM_A2)CSCH2  =   status;
}

void setPGA(char PGAnum, char gain) {
    set_CS(PGAnum,0);
    spi_write16(0x4000 | gain);
    set_CS(PGAnum,1);
}

void delayTMR4(int n){
    _T4IF=0;
    PR4=n;
    while(!_T4IF);
}
void init() {

    /*Switching clock to external crystal, and enabling PLL*/
    PLLFBD = 62; // prescale by a factor of 64
    CLKDIVbits.PLLPOST = 0; // postscale by 2
    CLKDIVbits.PLLPRE = 1; //divide by 3

    // Initiate Clock Switch to Primary Oscillator with PLL (NOSC=0b011)
    __builtin_write_OSCCONH(0x03); 
    __builtin_write_OSCCONL(OSCCON | 0x01);
    while (OSCCONbits.COSC != 0b011); // Wait for Clock switch to occur
    while (OSCCONbits.LOCK != 1); //// Wait for PLL to lock
    /*----Clock switching complete. Fosc=128MHz . Fcy = 64MHz------*/
    

    PTGCONbits.PTGWDT = 0; //disable peripheral trigger generator watchdog timer
    //RCONbits.SWDTEN=0;  //disable software watchdog

    _TRISB14 = 0; _LATB14=1; //CCS . OFF
    
    //Digital outputs
    _TRISB9 = 0; //SQR1 RP41
    _TRISC6 = 0; //OD1  RP54
    _TRISC7 = 0; //PDC1 RP55
    _TRISC8 = 0; //PDC2 RP56
    _TRISC9 = 0; //W1   RP57
    _TRISB6 = 0; //SQ2  RP38

    _TRISB10 = 1; //OD1  RP42
    _TRISB11 = 1; //PDC1 RP43
    //_TRISB12 = 1; //PDC2 RP44 NC
    //_TRISB13 = 1; //ID1 RP45  NC

    //ADC inputs. 4 channels
    _TRISA0 = 1; _TRISA1 = 1;
    _TRISB1 = 1;_TRISB0=1; 


    //------------Initialize SPI pins for PGAs -------------------------
    
    _TRISA10 = 0;    _LATA10 = 1;                //CS-1 kept high.
    _TRISA7 = 0;     _LATA7 = 1;                 //CS-2 kept high.

    _TRISB5 = 0;     _LATB5=1;                   //LEDPIN

    _TRISA4 = 0;                                 //SDO1
    _ANSA4 = 0;                                  //make A4 digital out
    _TRISC3 = 0;                                 //SCK1 output

    //-------CTMU input PIN RP35 (C1IN1-)--------
    // Output pin for constant voltage mode capacitance measurement
    _TRISB3=1; _ANSB3=1;           //SET B3 as an input pin in analog mode . CTMU
    _TRISB2=1; _ANSB2=1;           //SET B2 as an input pin in analog mode . PDC2 /AN4
    _TRISC0=1; _ANSC0=1;           //CCS monitoring
    _TRISC1=1; _ANSC1=1;           //SEN monitoring
    _TRISC2=1; _ANSC2=1;           //SET C2 as an input pin in analog mode . PDC1 / AN8


    _TRISA9 = 1;                   //SET A9 as an input pin . Charging capacitors in constant voltage mode
     
    _U1RXIF=0;_U1RXIE =0;  //disable receive interrupt for uart1
    _U2RXIF=0;_U2RXIE =0;  //disable receive interrupt for uart2

    disableCTMUSource();
    configureADC();

    initI2C();
    initSPI();
    initDAC();
    
}

void setFlashPointer(BYTE location){ //Calibration constants are stored in flash mem
    if(location<20){
        _init_prog_address(p, dat1[0]);
        p+=0x800*location;
    }

}

/*-----------------------square wave output OC1,TMR1, -----------------*/
void sqr1(unsigned int wavelength,unsigned int high_time,BYTE scaling) {
    OC1R = high_time-1;
    PR1 = wavelength-1;

    OC1CON1 = 6; //Edge aligned PWM
    OC1CON1bits.OCTSEL = 4;
    T1CONbits.TCKPS = scaling&0x3;
    OC1CON2 = 11; //01011 = Timer1 synchronizes or triggers OCx (default)
    T1CONbits.TON = 1;
    RPOR3bits.RP41R = 0x10; //square wave pin(RB9) mapped to 0b010001 (output compare 1 )
/*-----------------------square wave output-----------------*/

}

/*-----------------------DAC output-------OC2,OC3, RC8,RB10----------*/
void initDAC() {
    OC3R = 1;OC2R = 1;
    OC3CON1 = 6;OC2CON1 = 6; //Edge aligned PWM
    
    OC3CON1bits.OCTSEL = 7;OC2CON1bits.OCTSEL = 7;
    OC3CON2 = 0x1F;OC2CON2 = 0x1F;     
    OC3RS = DAC_WAVELENGTH;  OC2RS = DAC_WAVELENGTH; 

    RPOR6bits.RP56R = 0x11; //RP56/RC8/PV1 mapped to  (output compare 2 )
    RPOR5bits.RP55R = 0x12; //RP55/RBC7/PV2 mapped to   (output compare 3 )
}

/*-----------------------square wave output OC4,TMR4,(shared with sine) -----------------*/
void sqr2(unsigned int wavelength,unsigned int high_time,BYTE scaling) {
    RPOR6bits.RP57R = 0x0;  //disable sine output
    DMA3CONbits.CHEN = 0;   //disable sine output

    OC4R = high_time-1;
    PR4 = wavelength-1;

    OC4CON1 = 6; //Edge aligned PWM
    OC4CON1bits.OCTSEL = 2; //timer 4 is the clock source
    T4CONbits.TCKPS = scaling&0x3;
    OC4CON2 = 0b1110; //01110 = Timer4 synchronizes or triggers OCx (default)
    T4CONbits.TON = 1;
    RPOR2bits.RP38R = 0x13; //square wave pin(PGC/RB6/RP38) mapped to 0b010011 (output compare 4 )
    
    /*-----------------------square wave output-----------------*/

}

/*-----------------------sine wave output-----------T4,OC4,DMA3------*/
void sineWave1(unsigned int wavelength,BYTE highres){
    T4CONbits.TON= 0;
    _DMA3IF = 0;    _DMA3IE = 0;
    DMA3CONbits.CHEN = 0;
    if(highres&1){
        OC4R =HIGH_RES_WAVE>>1;     OC4RS = HIGH_RES_WAVE;
    }else{
        OC4R =LOW_RES_WAVE>>1;      OC4RS = LOW_RES_WAVE;        
    }
    OC4CON2 = 0;
    OC4CON1 = 6; //Edge aligned PWM
    OC4CON2bits.SYNCSEL = 0x1F;  //OCRS compare used for sync
    OC4CON1bits.OCTSEL = 7;     //Fp used as clock

    
    DMA3CONbits.AMODE = 0; // Configure DMA for Register Indirect mode with post-increment
    DMA3CONbits.SIZE = 0;  //word transfer
    DMA3CONbits.MODE = 0; // Configure DMA for Continuous mode
    DMA3CONbits.DIR = 1; // RAM-to-Peripheral data transfers
    DMA3PAD = (volatile unsigned int)&OC4R; // Point DMA to OC3R
    DMA3REQ = 0b11011;    //timer 4 triggers DMA


    if(highres&1){
        DMA3STAH = __builtin_dmapage (&sineTable1);
        DMA3STAL = __builtin_dmaoffset (&sineTable1);
        DMA3CNT = WAVE_TABLE_FULL_LENGTH-1; // total table size -1/  DMA requests
    }else{
        DMA3STAH = __builtin_dmapage (&sineTable1_short);
        DMA3STAL = __builtin_dmaoffset (&sineTable1_short);
        DMA3CNT = WAVE_TABLE_SHORT_LENGTH-1; // total table size -1/  DMA requests
    }


    _DMA3IF = 0;    _DMA3IE = 1;
    DMA3CONbits.CHEN = 1;

    T4CONbits.TCKPS = (highres>>1)&3;
    PR4 = wavelength;
    T4CONbits.TON= 1;
    
    RPOR2bits.RP38R = 0; // square wave 2 pin(RB6/RP38) disconnected from output compare
    RPOR6bits.RP57R = 0x13;  //Sine 1 Mapping OC4 output to square wave 4 SQR4 (C9)
}

void initUART(unsigned int BAUD) {
    /*---------UART------------*/
    TRISBbits.TRISB8 = 1; // B8 set as input(RX). connected to TX of MCP2200
    ANSELBbits.ANSB8 = 0; // set B8 as digital input.
    TRISBbits.TRISB7 = 0; // set as output. connected to RX of MCP2200

    RPOR2bits.RP39R = 0x01; //Map B7(RP39) to UART TX
    RPINR18bits.U1RXR = 0x28; //Map B8(RP40) to UART1 RX

    U1MODEbits.STSEL = 0; //1 stop bit
    U1MODEbits.PDSEL = 0; //no parity, 8-data bits
    U1MODEbits.ABAUD = 0; //disable auto-baud
    U1MODEbits.BRGH = 1; //high speed mode
    U1BRG = BAUD;
    U1MODEbits.UEN = 0;
    U1MODEbits.RTSMD = 1;

    U1STAbits.URXISEL = 0; //interrupt on 1 char recv

    //IEC0bits.U1TXIE = 1; //enable TX interrupt

    U1MODEbits.UARTEN = 1; //enable UART
    U1STAbits.UTXEN = 1; //enable UART TX

    U1MODEbits.URXINV = 0;

    DELAY_105uS
    while(hasChar())getChar(); //clear buffer

}

void setADCMode(BYTE mode,BYTE chosa,BYTE ch123sa){
    if(ADC_MODE == mode && chosa==CHOSA && ch123sa == CH123SA)return;
    else{
        if(CHOSA==7 || CHOSA == 5)DisableComparator();
        ADC_MODE = mode;CHOSA=chosa;CH123SA=ch123sa;
        if(mode == ADC_10BIT_SIMULTANEOUS)initADC10();
        else if(mode == ADC_10BIT_DMA)initADCDMA(0);  //12 bit mode disabled
        else if(mode == ADC_12BIT_DMA)initADCDMA(1);  //12 bit mode
        else if(mode == ADC_12BIT)initADC12();
        else if(mode == ADC_12BIT_SCOPE)initADC12bit_scope();
        else if(mode == ADC_12BIT_AVERAGING)initADC12_averaging16();
        else if(mode == ADC_CTMU)initADCCTMU();


    }
}

void setupADC10() {
    T5CONbits.TCKPS = 1;
    PR5 = ADC_DELAY-1;
    TMR5 = 0x0000;
    T5CONbits.TON = 1;

}

void preciseDelay(int t){
    T5CONbits.TON = 0;
    T5CONbits.TCKPS = 2;
    PR5 = t-1;
    TMR5 = 0x0000;
    _T5IF=0;
    T5CONbits.TON = 1;
    while(!_T5IF);
    T5CONbits.TON = 0;
    
} 

void enableADCDMA(){
    AD1CON1bits.ADDMABM=1;AD1CON4bits.ADDMAEN=1;
}

void disableADCDMA(){
    AD1CON1bits.ADDMABM=0;AD1CON4bits.ADDMAEN=0;DMA0CONbits.CHEN = 0; 
}

void PrepareTrigger(void) {
    TRIGGER_WAITING=0; TRIGGER_READY=0;TRIGGERED=0;
}

void initADC10(void) {
    /* Set port configuration */
    AD1CON1 = 0;
    disableADCDMA();
   /* Initialize ADC module */
    AD1CON1bits.AD12B = 0;
    AD1CON1bits.SSRCG = 0;
    AD1CON1bits.SSRC = 4; //Timer5 compare starts conversion
    AD1CON1bits.ASAM = 1; //auto sampling

    AD1CON1bits.SIMSAM = 1; //simultaneous sampling
    AD1CHS0bits.CH0SA = CHOSA; //AN3 - CH0
    AD1CHS0bits.CH0NA = 0;
    AD1CHS123bits.CH123SA = CH123SA; //AN0 -> CH1 , AN1 -> ch2, AN2 -> ch3
    AD1CHS123bits.CH123NA = 0; //-ve of CH1,2,3 to -vref
    AD1CON2bits.SMPI = 0; //generate interrupt after converting all chans

    AD1CON4 = 0x0000;
    AD1CSSH = 0x0000;
    AD1CSSL = 0x0000;

    /* Assign MUXA inputs for block read */
    _AD1IF = 0;  _AD1IE = 0; //disable ADC interrupt until required
    //AD1CON3bits.SAMC = 0; // SAMC - Sample for (x+1)*Tad before triggering conversion (TMR5 will decide this here)
    AD1CON3bits.ADCS = 1; // Conversion clock x*Tp
    AD1CON3bits.ADRC = 0; //use clock derived from system clock

    AD1CON1bits.ADON = 1;
    Delay_us(20);

    T5CONbits.TON = 0;
    T5CONbits.TSIDL = 1;
    T5CONbits.TCKPS = 1;
    TMR5 = 0x0000;
    T5CONbits.TON = 1;
    _T5IF = 0;
    _T5IE = 0;
}

void initADCDMA(BYTE bits) {
    /* Set port configuration */
    AD1CON1 = 0;
    /* Initialize ADC module */
    AD1CON1bits.AD12B = bits;
    AD1CON1bits.SSRCG = 0;
    AD1CON1bits.SSRC = 4; //Timer5 compare starts conversion
    AD1CON1bits.ASAM = 1; //auto sampling

    AD1CON1bits.SIMSAM = 1; //simultaneous sampling
    AD1CHS0bits.CH0SA = CHOSA; 
    AD1CHS0bits.CH0NA = 0;
    AD1CHS123bits.CH123SA = CH123SA; //AN0 -> CH1 , AN1 -> ch2, AN2 -> ch3
    AD1CHS123bits.CH123NA = 0; //-ve of CH1,2,3 to -vref
    AD1CON2bits.SMPI = 0; //generate interrupt after converting all chans

    AD1CON4 = 0x0000;
    AD1CSSH = 0x0000;
    AD1CSSL = 0x0000;

    /* Assign MUXA inputs for block read */
    _AD1IF = 0;  _AD1IE = 0; //disable ADC interrupt until required
    AD1CON3bits.ADCS = 1; // Conversion clock x*Tp
    AD1CON3bits.ADRC = 0; //use clock derived from system clock

    AD1CON1bits.ADON = 1;
    Delay_us(20);

    T5CONbits.TON = 0;
    T5CONbits.TSIDL = 1;
    T5CONbits.TCKPS = 1;
    TMR5 = 0x0000;
    _T5IF = 0; _T5IE = 0;

    DMA0CONbits.CHEN = 0; 

    DMA0CONbits.AMODE = 0b00; // Register indirect with post increment
    DMA0CONbits.MODE = 0b01;// One Shot, Ping-Pong mode Disabled
    DMA0CONbits.DIR = 0;// Peripheral to RAM
    DMA0REQ = 0b1101; // Select ADC module as DMA request source


    DMA_MODE = DMA_LA_ONE_CHAN;
    enableADCDMA();

}

void initADC12bit_scope(void) {
    /* Set port configuration */
    disableADCDMA();

    AD1CON1bits.ADON = 0;
    /* Initialize ADC module */
    AD1CON1bits.AD12B = 1;
    AD1CON1bits.SSRCG = 0;
    AD1CON1bits.SSRC = 4; 
    AD1CON1bits.ASAM = 1; 

    AD1CHS0bits.CH0SA = CHOSA;
    AD1CHS0bits.CH0NA = 0;
    AD1CON2bits.SMPI = 0;

    AD1CON4 = 0x0000;
    AD1CSSH = 0x0000;
    AD1CSSL = 0x0000;

    /* Assign MUXA inputs for block read */
    _AD1IF = 0;
    _AD1IE = 0; //disable ADC interrupt until required
    AD1CON3bits.ADCS = 9; // Conversion clock x*Tp
    AD1CON3bits.ADRC = 0; //use clock derived from system clock

    AD1CON1bits.ADON = 1;
    Delay_us(20);

    T5CONbits.TON = 0;
    T5CONbits.TSIDL = 1;
    T5CONbits.TCKPS = 1;
    TMR5 = 0x0000;
    T5CONbits.TON = 1;
    _T5IF = 0;
    _T5IE = 0;
}

void initADC12(void) {
    _AD1IF = 0;  _AD1IE = 0;
    disableADCDMA();

    AD1CON2 = 0;
    AD1CON4 = 0x0000;
    AD1CSSH = 0x0000;
    AD1CSSL = 0x0000;

    AD1CON1bits.ADON = 0;

    AD1CON1bits.AD12B = 1;
    AD1CON1bits.ADSIDL = 0; 
    AD1CON3bits.ADRC = 1; //use internal clock
    AD1CON1bits.SSRCG = 0;
    AD1CON2bits.CHPS =0;
    AD1CHS0bits.CH0SA = CHOSA; //AN3 - CH0
    AD1CHS0bits.CH0NA = 0;
    AD1CON1bits.SSRC = 7; //Internal counter ends sampling, starts conversion
    //AD1CON1bits.SIMSAM = 0; //simultaneous sampling  .  Not applicable for single channel sampling
    AD1CON1bits.ASAM = 0; // No auto sampling
    AD1CON2bits.SMPI = 0; //generate interrupt after argument+1 conversion

    /* Assign MUXA inputs for block read */
    //AD1CHS0bits.CH0SA = channel; //AN<channel> connected to CH0
    AD1CON3bits.SAMC = 0x1f; // Sample for (x+1)*Tad before triggering conversion
    AD1CON3bits.ADCS = 9; // Conversion clock x*Tp

    //AD1CON2bits.CHPS = 0; //unimplemented in 12 bit mode. read as 0
    AD1CON1bits.ADON = 1;
    Delay_us(20);
}

void initADC12_averaging16() {
            _AD1IF = 0; _AD1IE = 0;                                             //disable ADC interrupts
            disableADCDMA();

            AD1CON1bits.ADON = 0;                                               //turn off ADC.
            AD1CON2 = 0;
            AD1CON4 = 0x0000;
            AD1CSSH = 0x0000;
            AD1CSSL = 0x0000;
            AD1CON1bits.AD12B = 1;                                              //12 bit mode
            AD1CON1bits.ADSIDL = 0;                                             //continue operation in idle
            AD1CON3bits.ADRC = 0;                                               //do not use internal clock
            AD1CON1bits.SSRCG = 0;                                              
            AD1CON2bits.CHPS =0;
            /* Assign MUXA inputs for block read */
            AD1CHS0bits.CH0SA = CHOSA;
            AD1CHS0bits.CH0NA = 0;
            AD1CON1bits.SSRC = 7;                                               //Internal counter ends sampling, starts conversion (SSRCG=0)

            AD1CON3bits.SAMC = 0x10; // Sample for (x+1)*Tad before triggering conversion
            AD1CON3bits.ADCS = 0xA; // Conversion clock Tad = ADCS*Tp(15nS))
            AD1CON2bits.SMPI = 15;   //generate interrupt after argument+1 conversions
}

unsigned int get_voltage_summed(BYTE channel){
            setADCMode(ADC_12BIT_AVERAGING,channel,0);
            AD1CON1bits.ADON = 1; Delay_us(20);   //Turn on the ADC
            AD1CON1bits.ASAM = 1; // auto sampling
            _AD1IF=0;while(!_AD1IF);_AD1IF=0;
            while(!AD1CON1bits.DONE);//wait for conversion
            AD1CON1bits.ASAM = 0; //stop auto sampling
            AD1CON1bits.ADON = 0;

            return (ADC1BUF0)+(ADC1BUF1)+(ADC1BUF2)+(ADC1BUF3)+(ADC1BUF4)+(ADC1BUF5)+(ADC1BUF6)+(ADC1BUF7)
                    +(ADC1BUF8)+(ADC1BUF9)+(ADC1BUFA)+(ADC1BUFB)+(ADC1BUFC)+(ADC1BUFD)+(ADC1BUFE)+(ADC1BUFF);

}

unsigned int get_voltage(BYTE channel){
            AD1CHS0bits.CH0SA = channel; //AN<channel> connected to CH0
            AD1CON1bits.SAMP = 1; //start sampling
            while (!AD1CON1bits.DONE);
            return ADC1BUF0;
}

void configureADC() {
    ANSELB = ANSELC = 0x0000;
    ANSELAbits.ANSA0 = 1;    ANSELAbits.ANSA1 = 1;    ANSELBbits.ANSB0 = 1;   ANSELBbits.ANSB1 = 1;   ANSELBbits.ANSB2 = 1;
    ANSELBbits.ANSB3 = 1;    ANSELCbits.ANSC0 = 1;    ANSELCbits.ANSC1 = 1;   ANSELCbits.ANSC2 = 1;
}

void Delay_us(unsigned int delay) {
    unsigned int i;
    for ( i = 0; i < delay; i++) {
        __asm__ volatile ("repeat #63");
        __asm__ volatile ("nop");
    }
}

void Delay_ms(unsigned int delay) {
    unsigned int i,i2;
    for ( i2 = 0; i2 < delay; i2++){
        for ( i = 0; i < 860; i++) {
            __asm__ volatile ("repeat #63");
            __asm__ volatile ("nop");
        }
    __asm__ volatile ("CLRWDT"); //to prevent watchdog reset
    }
}

void read_all_from_flash(_prog_addressT pointer) {
    unsigned int bytes_to_read = _FLASH_ROW * 16;       // _FLASH_ROW*8 integers = twice as many bytes
    _prog_addressT p1, p2;
    p1 = pointer;
    for (i = 0; i < bytes_to_read / 2; i++)dest[i] = 0; //clear buffer before filling with values read from flash
    p2 = _memcpy_p2d16(&dest, pointer, bytes_to_read);
    Nop();
    Nop();

}

void load_to_flash(_prog_addressT pointer, BYTE location, unsigned int * blk) {
    /*Write to locations of 16bytes each (store 8 integers as a string, or 16 BYTES ...)*/
    char bytes_to_write = 16;
    _prog_addressT p;
    /*The storage architecture only allows erasing a whole page(1024) at a time. So we must make
     a copy of the data in the RAM, change the locations we need to access, and write the whole page back*/

    read_all_from_flash(pointer);
    /*-----------fetch bytes_to_write characters----------*/
    for (i = 0; i < bytes_to_write / 2; i++) {
        dest[location * 8 + i] = blk[i];
    }
    /*------write the copy back into the FLASH MEMORY------*/
    unsigned int dat1, dat2;
    _erase_flash(pointer); /* erase a page */
    for (i = 0; i < _FLASH_ROW * 4; i += 1) /*combine two ints each for each word32 write*/ {
        dat1 = dest[2 * i];
        dat2 = dest[2 * i + 1];
        p = pointer + (4 * i);
        _write_flash_word32(p, dat1, dat2);
    }


}

void read_flash(_prog_addressT pointer, BYTE location) {
    read_all_from_flash(pointer);

    for (i = 0; i < 8; i++) {
        blk[i] = dest[location * 8 + i];
    }
}

bool hasChar() {
    return U1STAbits.URXDA;
}

void sendChar(BYTE val) {
    while (U1STAbits.UTXBF); //wait for transmit buffer empty
    U1TXREG = val;
}

void sendInt(unsigned int val) {
    while (U1STAbits.UTXBF); //wait for transmit buffer empty
    U1TXREG = val & 0xff;
    while (U1STAbits.UTXBF); //wait for transmit buffer empty
    U1TXREG = (val >> 8)&0xff;
}

void sendLong(unsigned int lsb,unsigned int msb) {
    while (U1STAbits.UTXBF); //wait for transmit buffer empty
    U1TXREG = lsb & 0xff;
    while (U1STAbits.UTXBF); //wait for transmit buffer empty
    U1TXREG = (lsb >> 8)&0xff;
    while (U1STAbits.UTXBF); //wait for transmit buffer empty
    U1TXREG = msb & 0xff;
    while (U1STAbits.UTXBF); //wait for transmit buffer empty
    U1TXREG = (msb >> 8)&0xff;
}

void ack(BYTE response) {
    while (U1STAbits.UTXBF); //wait for transmit buffer empty
    U1TXREG = response;
}

char getChar() {
    while (!hasChar());
    if (U1STAbits.FERR == 1) {
        U1RXREG;
        U1STAbits.OERR = 0;
        return 0;
    }
    return U1RXREG;
}

unsigned int getInt() {
    c1 = getChar()&0xFF;
    c2 = getChar()&0xFF;
    return (c2 << 8) | c1;
}

void initI2C(void) {

    _TRISB4 = 1; // set SCL and SDA pins as inputs.
    _TRISA8 = 1;
    ODCBbits.ODCB4=1;
    ODCAbits.ODCA8=1;
    CNPUBbits.CNPUB4 = 1;
    CNPUAbits.CNPUA8 = 1;
    I2C2CON=0;
    
    Delay_us(1000);
    I2C2CONbits.I2CEN = 0;
    I2C2BRG = I2C_BRGVAL;

    I2C2STAT = 0b0000000000000000;

    I2C2CONbits.DISSLW = 0; //disable slew rate
    I2C2CONbits.I2CEN = 1; //enable. configure SDA, SCL as serial
    Delay_us(1000);

}

void I2CStart() {
    I2C2CONbits.SEN = 1; /* Start condition enabled */
    tmp_int1=1000;
    while (I2C2CONbits.SEN && tmp_int1--)Delay_us(1); /* wait for ack data to send on bus */
    /* wait for start condition to finish */
}

void I2CStop() {
    I2C2CONbits.PEN = 1; /* Stop condition enabled */
    tmp_int1=1000;
    while (I2C2CONbits.PEN && tmp_int1--)Delay_us(1); /* wait for stop cond to finish */

    /* PEN automatically cleared by hardware */
}

void I2CRestart() {
    I2C2CONbits.RSEN = 1; /* Repeated start enabled */
    tmp_int1=1000;
    while (I2C2CONbits.RSEN && tmp_int1--)Delay_us(1); /* wait for condition to finish */

}

void I2CAck() {
    I2C2CONbits.ACKDT = 0; /* Acknowledge data bit, 0 = ACK */
    I2C2CONbits.ACKEN = 1; /* Ack data enabled */

    tmp_int1=1000;
    while (I2C2CONbits.ACKEN && tmp_int1--)Delay_us(1); /* wait for ack data to send on bus */
}

void I2CNak() {
    I2C2CONbits.ACKDT = 1; /* Acknowledge data bit, 1 = NAK */
    I2C2CONbits.ACKEN = 1; /* Ack data enabled */
    tmp_int1=1000;
    while (I2C2CONbits.ACKEN && tmp_int1--)Delay_us(1); /* wait for ack data to send on bus */

}

void I2CWait() {
    tmp_int1=1000;
    while (I2C2STATbits.TBF && tmp_int1--)Delay_us(1);
    /* wait for any pending transfer */
}

void I2CSend(BYTE dat) {
    I2C2TRN = dat; /* Move data to SSPBUF */
    tmp_int1=1000;
    while (I2C2STATbits.TRSTAT && tmp_int1--)Delay_us(1);/* wait till complete data is sent from buffer */

    I2CWait(); /* wait for any pending transfer */
}

BYTE I2CRead(BYTE ack){
    BYTE retval;
    I2CWait();
    I2C2CONbits.RCEN=1;

    tmp_int1=1000;
    while (I2C2CONbits.RCEN && tmp_int1--)Delay_us(1);
    while ((!I2C2STATbits.RBF) && tmp_int1--)Delay_us(1);

    retval = I2C2RCV;
    if(ack)I2CAck();
    else I2CNak();
    return retval;
    
}

void logit(char *str){
    while(*str!='\0'){
        *error_writepos++=*str++;
        if(error_writepos==&errors[ERROR_BUFFLEN])
            error_writepos=&errors[0];
    }
}



