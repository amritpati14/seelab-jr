/*
 * File:   globals.c
 * Author: jithin
 *
 * Created on 2 October, 2016, 10:09 AM
 */


#include "xc.h"
#include"functions.h"

/*Global Variables*/


__prog__ unsigned int __attribute__((section("CALIBS"), space(prog), aligned(_FLASH_PAGE * 2))) dat1[15][_FLASH_PAGE];

int __attribute__((section("adcbuff"),far)) ADCbuffer[BUFFER_SIZE];
int *buff0,*buff1,*endbuff,*buff2,*buff3;
int  *buffpointer, *endpointer, dma_channel_length=10,samples=0,I2CSamples=0;
BYTE *bytebuff1,*bytebuff2,ca,cb;

unsigned int dest[_FLASH_ROW * 8];
int ulsb,umsb; //DAC_OFFSETS[4],

/*-----SPI VARIABLES-------*/
BYTE location, value,ADC_MODE=NOT_READY,DMA_MODE=NOT_READY,ADC_STREAMING=false;

/*------UART VARIABLES-----*/
unsigned int I2C_BRGVAL = 0x272;

/*------LOGIC ANALYZER VARIABLES-----*/
BYTE INITIAL_DIGITAL_STATES=0,INITIAL_DIGITAL_STATES_ERR=0,DIGITAL_TRIGGER_CHANNEL=32,DIGITAL_TRIGGER_STATE=0,b1,b2,COMPARATOR_CONFIG=7|(3<<4),conversion_done = 1,I2CConvDone = 1;
unsigned int i, lsb, msb, blk[8], c1, c2,adval,tmp_int1,tmp_int2,tmp_int3,tmp_int4,tmp_int5,tmp_int6;

unsigned int LAFinished = 1, LASamples;
unsigned int samples_to_fetch = BUFFER_SIZE, I2CTotalSamples = BUFFER_SIZE;
unsigned long val,l1,l2;
BYTE DIN_REMAPS[] = {ID1_REMAP,OD1_REMAP,PDC1_REMAP,PDC2_REMAP,COMP4_REMAP,RP41_REMAP,FREQ_REMAP},LAM1=0,LAM2=0,LAM3=0,LAM4=0;

/*-----OSCILLOSCOPE VARIABLES-------*/

BYTE ADC_CHANNELS = 0,CH123SA=0,CHOSA=3; // CH1 only
BYTE TRIGGER_CHANNEL=0,TRIGGERED=0,TRIGGER_READY=0,SH=5,ICG=15,I2C_TRIGGER_CHANNEL=0,I2C_TRIGGERED=0,I2C_TRIGGER_READY=0, I2C_SCOPE_LOCATION=0x00,I2C_SCOPE_ADDRESS=0x00,I2C_SCOPE_BYTES=0;
unsigned int TRIGGER_TIMEOUT=100,TRIGGER_WAITING=0,TRIGGER_LEVEL=0,TRIGGER_PRESCALER=0;
unsigned int ADC_DELAY = 5;

BYTE frequency_scaling=1,frequency_ready=false;
unsigned int freq_lsb,freq_msb,freq2_lsb,freq2_msb;
_prog_addressT p,pProg;

/*--------Stepper Motor--------*/
BYTE motor_phases[] = {12,6,3,9},current_motor_phase = 0;


/*--------Error handling definitions------*/
char errors[ERROR_BUFFLEN],tmpstr[25];
char *error_readpos=&errors[0],*error_writepos=&errors[0];


int  __attribute__((section("sine_table1"))) sineTable1[]  ={
256, 252, 249, 246, 243, 240, 237, 234, 230, 227, 224, 221, 218, 215, 212, 209, 206, 203, 200, 196, 193, 190, 187, 184, 181, 178, 175, 172, 169, 166, 164, 161, 158, 155, 152, 149, 146, 143, 141, 138, 135, 132, 130, 127, 124, 121, 119, 116, 114, 111, 108, 106, 103, 101, 98, 96, 93, 91, 89, 86, 84, 82, 79, 77, 75, 73, 70, 68, 66, 64, 62, 60, 58, 56, 54, 52, 50, 48, 47, 45, 43, 41, 40, 38, 36, 35, 33, 32, 30, 29, 27, 26, 25, 23, 22, 21, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 8, 7, 6, 6, 5, 4, 4, 3, 3, 2, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5, 6, 6, 7, 8, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 29, 30, 32, 33, 35, 36, 38, 40, 41, 43, 45, 47, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 73, 75, 77, 79, 82, 84, 86, 89, 91, 93, 96, 98, 101, 103, 106, 108, 111, 114, 116, 119, 121, 124, 127, 130, 132, 135, 138, 141, 143, 146, 149, 152, 155, 158, 161, 164, 166, 169, 172, 175, 178, 181, 184, 187, 190, 193, 196, 200, 203, 206, 209, 212, 215, 218, 221, 224, 227, 230, 234, 237, 240, 243, 246, 249, 252, 256, 259, 262, 265, 268, 271, 274, 277, 281, 284, 287, 290, 293, 296, 299, 302, 305, 308, 311, 315, 318, 321, 324, 327, 330, 333, 336, 339, 342, 345, 347, 350, 353, 356, 359, 362, 365, 368, 370, 373, 376, 379, 381, 384, 387, 390, 392, 395, 397, 400, 403, 405, 408, 410, 413, 415, 418, 420, 422, 425, 427, 429, 432, 434, 436, 438, 441, 443, 445, 447, 449, 451, 453, 455, 457, 459, 461, 463, 464, 466, 468, 470, 471, 473, 475, 476, 478, 479, 481, 482, 484, 485, 486, 488, 489, 490, 492, 493, 494, 495, 496, 497, 498, 499, 500, 501, 502, 503, 503, 504, 505, 505, 506, 507, 507, 508, 508, 509, 509, 509, 510, 510, 510, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 510, 510, 510, 509, 509, 509, 508, 508, 507, 507, 506, 505, 505, 504, 503, 503, 502, 501, 500, 499, 498, 497, 496, 495, 494, 493, 492, 490, 489, 488, 486, 485, 484, 482, 481, 479, 478, 476, 475, 473, 471, 470, 468, 466, 464, 463, 461, 459, 457, 455, 453, 451, 449, 447, 445, 443, 441, 438, 436, 434, 432, 429, 427, 425, 422, 420, 418, 415, 413, 410, 408, 405, 403, 400, 397, 395, 392, 390, 387, 384, 381, 379, 376, 373, 370, 368, 365, 362, 359, 356, 353, 350, 347, 345, 342, 339, 336, 333, 330, 327, 324, 321, 318, 315, 311, 308, 305, 302, 299, 296, 293, 290, 287, 284, 281, 277, 274, 271, 268, 265, 262, 259};


int  __attribute__((section("sine_table1_short"))) sineTable1_short[]  ={
      32, 26, 20, 14, 9, 5, 2, 1, 0, 1, 2, 5, 9, 14, 20, 26, 32, 38, 44, 50, 55, 59, 62, 63, 64, 63, 62, 59, 55, 50, 44, 38};




