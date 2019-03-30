/*****************************************************************************
* audio.cpp
*****************************************************************************/

/*
Program demonstrates a simple audio in-out capability of the ADSP-BF706 EZ-KIT Mini
evaluation system using a sample-by-sample algorithm. The program is completely
self-contained, using only the header cdefBF706.h for the addresses of the BF706
registers.

Author: Patrick Gaydecki
Date: 15.12.2016

Extension: audio-processing is called by the interrupt handler of the SPORT0
receive DMA interrupt. No polling is needed. The main loop can be used to 
process other tasks.

Author: Uwe Simmer
Date: 19.08.2017

This software is public domain
*/

#include <stdio.h>
#include <cdefBF706.h>
#include <services/int/adi_int.h>
#include "adi_initialize.h"
//#include <stdlib.h> 		//for using: int abs(int _i);

#include "Functions_32bit.h"
#include "WaveletCodecBlockBased.h"

//uint16_t prand_seed[8] = {1,2,3,4,8,7,6,5};       // used in function prand()

void write_TWI(uint16_t, uint8_t);
void configure_CODEC(void);
void init_interrupts(void);
void configure_SPORT(void);
void init_SPORT_DMA(void);
void enable_SPORT(void);
void disable_SPORT(void);
void SPORT0_RX_interrupt_handler(uint32_t iid, void *handlerArg);

#define BUFFER_SIZE 20                  	// Size of buffer to be transmitted
#define BLOCKSIZE 20						// Size of buffer to be transmitted

static unsigned int estimator_weight_transmit[4] = {0, 0, 10, 0};
static unsigned int estimator_weight_receive[4]  = {0, 0, 0, 0};

static int32_t SP0A_buffer[BLOCKSIZE*2];  // DMA TX buffer
static int32_t SP0B_buffer[BLOCKSIZE*2];  // DMA RX buffer
static int32_t audio_buffer[BLOCKSIZE*2]; // Audio processing buffer

static int input_block[BLOCKSIZE];	// Block to process wavelet codec
static int output_block[BLOCKSIZE];	// Block to process wavelet codec

static unsigned int bit_reservoir = 148;		// ~7,4bit/sample = 355,2kbit/s
//static unsigned int bit_reservoir = 92;		// ~4,6bit/sample = 220,8kbit/s
//static unsigned int bit_reservoir = 64;		// ~3,2bit/sample = 153,6kbit/s
//static unsigned int bit_reservoir = 78;		// ~3,9bit/sample = 153,6kbit/s

unsigned int DITHER = 0;
unsigned int QUANTIZER = 0;
unsigned int OFFSET = 0;

unsigned int BIT_GROUP_ALLOCATION = 1;     // Switch between Greedy Bit Allocation (Ramstadt) and Bit Group Allocation
unsigned int estimation = 1;               // Switch between gain estimation and block gain
unsigned int limit_reservoir = 1;

unsigned int packetizing = 1;
unsigned int lostbit;                      // This flag is used for bit stuffing - we can avoid using the estimators here!

unsigned long long test;

unsigned int W2index0  = 1;     // Index for 10 W2
unsigned int W2index1  = 3;     // Index for 10 W2
unsigned int W2index2  = 5;     // Index for 10 W2
unsigned int W2index3  = 7;     // Index for 10 W2
unsigned int W2index4  = 9;     // Index for 10 W2
unsigned int W2index5  = 11;	// Index for 10 W2
unsigned int W2index6  = 13;	// Index for 10 W2
unsigned int W2index7  = 15;	// Index for 10 W2
unsigned int W2index8  = 17;	// Index for 10 W2
unsigned int W2index9  = 19;	// Index for 10 W2

unsigned int W1index0  = 0;  	// Index for 5 W1
unsigned int W1index1  = 4;     // Index for 5 W1
unsigned int W1index2  = 8;     // Index for 5 W1
unsigned int W1index3  = 12;	// Index for 5 W1
unsigned int W1index4  = 16;	// Index for 5 W1

unsigned int W0index0  = 2;     // Index for 3 W0
unsigned int W0index1  = 10;	// Index for 3 W0
unsigned int W0index2  = 18;	// Index for 3 W0

unsigned int V0index0  = 6;     // Index for 2 V0
unsigned int V0index1  = 14;	// Index for 2 V0

unsigned int allocateIndex;
unsigned int bits_per_subband[4] = {2, 3, 5, 10};     // Bits to alloc  ate to the subbands V0, W0, W1 and W2

unsigned int estimator_value_transmit[4];
unsigned int estimator_value_receive[4] = {0, 0, 0, 0};
unsigned int estimator_value_receive_old[4] = {0, 0, 0, 0};

unsigned int allocate_bits_transmit[4] = {0, 0, 0, 0};
unsigned int allocate_bits_receive[4] = {0, 0, 0, 0};

int 	W2, W1, W0, V0; 			// Estimator in Synthesis process
int 	leading_zeros_W2 = 0,
   		leading_zeros_W1 = 0,
   		leading_zeros_W0 = 0,
   		leading_zeros_V0 = 0;

unsigned int max_W2, max_W1, max_W0, max_V0;

unsigned int sigmaK_transmit[4] = {0, 0, 0, 0};
unsigned int sigmaK_receive[4]  = {0, 0, 0, 0};

unsigned long long int temp_data_transmit = 0;	// temporary "rescue" variable for data to be shifted (and maybe lost during shifting)
unsigned long long int temp_data_receive = 0;	// temporary "rescue" variable for data to be shifted (and maybe lost during shifting)

int i, k;

//typedef union {
//    unsigned long long 	myword[3];	// here are 3 64-bit unsigned integers for 192bit
//	unsigned char      	mybyte[21];	// here are 21 8-bit unsigned integers for 168bit
//} data;

data mydata_transmit;				// this is the data structure we use to pack variances and normalized quantized wavelet coefficients
data mydata_receive;				// this is the data structure we use to pack variances and normalized quantized wavelet coefficients


// Function write_TWI is a simple driver for the TWI. Refer to page 26–14 onwards
// of the ADSP-BF70x Blackfin+ Processor Hardware Reference, Revision 1.0.
void write_TWI(uint16_t reg_add, uint8_t reg_data) {
    int n;
    reg_add = (reg_add<<8) | (reg_add>>8);  // Reverse low order and high order bytes
    *pREG_TWI0_CLKDIV = 0x3232;             // Set duty cycle
    *pREG_TWI0_CTL = 0x8c;                  // Set prescale and enable TWI
    *pREG_TWI0_MSTRADDR = 0x38;             // Address of CODEC
    *pREG_TWI0_TXDATA16 = reg_add;          // Address of register to set, LSB then MSB
    *pREG_TWI0_MSTRCTL = 0xc1;              // Command to send three bytes and enable transmit
    for (n = 0; n < 8000; n++) {}           // Delay since CODEC must respond
    *pREG_TWI0_TXDATA8 = reg_data;          // Data to write
    for (n = 0; n < 10000; n++) {}          // Delay
    *pREG_TWI0_ISTAT = 0x050;               // Clear TXERV interrupt
    for (n = 0; n < 10000; n++) {}          // Delay
    *pREG_TWI0_ISTAT = 0x010;               // Clear MCOMP interrupt
}

// Function configure_CODEC initializes the ADAU1761 CODEC. Refer to the control register
// descriptions, page 51 onwards of the ADAU1761 data sheet.
void configure_CODEC() {
    write_TWI(0x4000, 0x01);                // Enable master clock, disable PLL
    write_TWI(0x400a, 0x0b);                // Set left line-in gain to 0 dB
    write_TWI(0x400c, 0x0b);                // Set right line-in gain to 0 dB
    write_TWI(0x4015, 0x01);                // Set serial port master mode
    write_TWI(0x4017, 0x00);                // Set CODEC default sample rate, 48 kHz
    write_TWI(0x4019, 0x63);                // Set ADC to on, both channels
    write_TWI(0x401c, 0x21);                // Enable left channel mixer
    write_TWI(0x401e, 0x41);                // Enable right channel mixer
    write_TWI(0x4023, 0xe7);                // Set left headphone volume to 0 dB
    write_TWI(0x4024, 0xe7);                // Set right headphone volume to 0 dB
    write_TWI(0x4029, 0x03);                // Turn on power, both channels
    write_TWI(0x402a, 0x03);                // Set both DACs on
    write_TWI(0x40f2, 0x01);                // DAC gets L, R input from serial port
    write_TWI(0x40f3, 0x01);                // ADC sends L, R input to serial port
    write_TWI(0x40f9, 0x7f);                // Enable all clocks
    write_TWI(0x40fa, 0x03);                // Enable all clocks
}

// Function init_interrupts enables the System Event Controller and installs the
// SPORT0 receive DMA interrupt handler
void init_interrupts(void) {
    *pREG_SEC0_GCTL  = ENUM_SEC_GCTL_EN;    // Enable the System Event Controller (SEC)
    *pREG_SEC0_CCTL0 = ENUM_SEC_CCTL_EN;    // Enable SEC Core Interface (SCI)

    // Set SPORT0_B DMA interrupt handler
    adi_int_InstallHandler(INTR_SPORT0_B_DMA, SPORT0_RX_interrupt_handler, 0, true);
}

// Function init_SPORT_DMA initializes the SPORT0 DMA0 and DMA1 in autobuffer mode.
// Refer to pages 19–39, and 19–49 of the ADSP-BF70x Blackfin+ Processor Hardware
// Reference, Revision 1.0.
void init_SPORT_DMA(void) {
    // SPORT0_A DMA Initialization
    *pREG_DMA0_CFG       = 0x00001220;      // SPORT0 TX, FLOW = autobuffer, MSIZE = PSIZE = 4 bytes
    *pREG_DMA0_ADDRSTART = SP0A_buffer;     // points to start of SPORT0_A buffer
    *pREG_DMA0_XCNT      = BLOCKSIZE*2;   // no. of words to transmit
    *pREG_DMA0_XMOD      = 4;               // stride

    // SPORT0_B DMA Initialization
    *pREG_DMA1_CFG       = 0x00101222;      // SPORT0 RX, DMA interrupt when x count expires
    *pREG_DMA1_ADDRSTART = SP0B_buffer;     // points to start of SPORT0_B buffer
    *pREG_DMA1_XCNT      = BLOCKSIZE*2;   // no. of words to receive
    *pREG_DMA1_XMOD      = 4;               // stride
}

// Function configure_sport initializes the SPORT0. Refer to pages 31-55, 31-63,
// 31-72 and 31-73 of the ADSP-BF70x Blackfin+ Processor Hardware Reference, Revision 1.0.
void configure_SPORT() {
    *pREG_SPORT0_CTL_A = 0x02011972;        // Set up SPORT0 (A) as TX to CODEC, I2S, 24 bits
    *pREG_SPORT0_DIV_A = 0x00400001;        // 64 bits per frame, clock divisor of 1
    *pREG_SPORT0_CTL_B = 0x00011972;        // Set up SPORT0 (B) as RX from CODEC, I2S, 24 bits
    *pREG_SPORT0_DIV_B = 0x00400001;        // 64 bits per frame, clock divisor of 1
}

// Function enable_SPORT enables DMA first and then SPORT
void enable_SPORT(void) {
    *pREG_DMA0_CFG |= ENUM_DMA_CFG_EN;
    *pREG_DMA1_CFG |= ENUM_DMA_CFG_EN;
    __builtin_ssync();

    *pREG_SPORT0_CTL_B |= 1;
    *pREG_SPORT0_CTL_A |= 1;
    __builtin_ssync();
}

// Function disable_SPORT disables SPORT first and then DMA
void disable_SPORT(void) {
    *pREG_SPORT0_CTL_B &= ~1;
    *pREG_SPORT0_CTL_A &= ~1;
    __builtin_ssync();

    *pREG_DMA0_CFG &= ~ENUM_DMA_CFG_EN;
    *pREG_DMA1_CFG &= ~ENUM_DMA_CFG_EN;
    __builtin_ssync();
}

// Function SPORT0_RX_interrupt_handler is called after a complete
// frame of input data has been received.
void SPORT0_RX_interrupt_handler(uint32_t iid, void *handlerArg) {

	int32_t i, k;

	#pragma vector_for
    for (k = 0; k < BUFFER_SIZE*2; k+=2) {
    	input_block[k>>1] = ((SP0B_buffer[k] + SP0B_buffer[k+1]) >> 1);  //-6dB after adding left and right channel
    }

	// Process

	// First Level Decomposition

	// Praediktionsschritt

	for (i = 1; i <= 17; i+=2) {
		input_block[i] -= ((input_block[i-1]>>1) + (input_block[i+1]>>1));
	}

	input_block[19] -= input_block[18];		// Randangepasstes Wavelet fuer Prediktion

	// Updateschritt

	input_block[0]  += input_block[1] >>1;	// Randangepasstes Wavelet fuer Update

	for (i = 1; i <= 17; i+=2) {
		input_block[i+1] += ((input_block[i]>>2) + (input_block[i+2]>>2));
	}

 	// Second Level Decomposition

	// Praediktionsschritt

	input_block[0]  -= input_block[2];		// Randangepasstes Wavelet fuer Prediktion

	for (i = 2; i <= 14; i+=4) {
		input_block[i+2] -= ((input_block[i] >>1) + (input_block[i+4]>>1));
	}

	// Updateschritt

	for (i = 2; i <= 14; i+=4) {
		input_block[i] += ((input_block[i-2]>>2) + (input_block[i+2]>>2));
	}

	input_block[18]  += input_block[16]>>1;	// Randangepasstes Wavelet fuer Update

	// Third Level Decomposition

	// Praediktionsschritt

	input_block[2]  -= input_block[6];		// Randangepasstes Wavelet fuer Prediktion

	input_block[10] -= ((input_block[6] >>1) + (input_block[14]>>1));

	input_block[18] -= input_block[14];		// Randangepasstes Wavelet fuer Prediktion

	// Updateschritt

	input_block[6]  += ((input_block[2] >>2) + (input_block[10]>>2));

	input_block[14] += ((input_block[10]>>2) + (input_block[18]>>2));

	// Audio coding starts here

	// Find maximum value for each subband

    max_W2 = max10(input_block[W2index0],
                   input_block[W2index1],
                   input_block[W2index2],
                   input_block[W2index3],
                   input_block[W2index4],
                   input_block[W2index5],
                   input_block[W2index6],
                   input_block[W2index7],
                   input_block[W2index8],
                   input_block[W2index9]);

    max_W1 = max5(input_block[W1index0],
	              input_block[W1index1],
	              input_block[W1index2],
	              input_block[W1index3],
	              input_block[W1index4]);

    max_W0 = max3(input_block[W0index0],
	              input_block[W0index1],
	              input_block[W0index2]);

	max_V0 = max2(input_block[V0index0],
	              input_block[V0index1]);

    // Calculate estimator "history" - might not be not needed for block signal processing
	sigmaK_transmit[3] = estimator(max_W2,sigmaK_transmit[3], estimator_weight_transmit[3]);
	sigmaK_transmit[2] = estimator(max_W1,sigmaK_transmit[2], estimator_weight_transmit[2]);
    sigmaK_transmit[1] = estimator(max_W0,sigmaK_transmit[1], estimator_weight_transmit[1]);
	sigmaK_transmit[0] = estimator(max_V0,sigmaK_transmit[0], estimator_weight_transmit[0]);

	// Caclulate leading zeroes for max value of each subband

	if (estimation) {
        leading_zeros_W2 = Count_Leading_Zeroes(sigmaK_transmit[3]);
        leading_zeros_W1 = Count_Leading_Zeroes(sigmaK_transmit[2]);
        leading_zeros_W0 = Count_Leading_Zeroes(sigmaK_transmit[1]);
        leading_zeros_V0 = Count_Leading_Zeroes(sigmaK_transmit[0]);
    }
    else {
        leading_zeros_W2 = Count_Leading_Zeroes(max_W2);
		leading_zeros_W1 = Count_Leading_Zeroes(max_W1);
		leading_zeros_W0 = Count_Leading_Zeroes(max_W0);
        leading_zeros_V0 = Count_Leading_Zeroes(max_V0);
    }

    estimator_value_transmit[3] = 32 - leading_zeros_W2;    // Estimator of W2
	estimator_value_transmit[2] = 32 - leading_zeros_W1;    // Estimator of W1
	estimator_value_transmit[1] = 32 - leading_zeros_W0;    // Estimator of W0
	estimator_value_transmit[0] = 32 - leading_zeros_V0;    // Estimator of V0

	input_block[W2index0] = Normalization(input_block[W2index0], leading_zeros_W2);
	input_block[W2index1] = Normalization(input_block[W2index1], leading_zeros_W2);
	input_block[W2index2] = Normalization(input_block[W2index2], leading_zeros_W2);
	input_block[W2index3] = Normalization(input_block[W2index3], leading_zeros_W2);
	input_block[W2index4] = Normalization(input_block[W2index4], leading_zeros_W2);
	input_block[W2index5] = Normalization(input_block[W2index5], leading_zeros_W2);
	input_block[W2index6] = Normalization(input_block[W2index6], leading_zeros_W2);
	input_block[W2index7] = Normalization(input_block[W2index7], leading_zeros_W2);
	input_block[W2index8] = Normalization(input_block[W2index8], leading_zeros_W2);
	input_block[W2index9] = Normalization(input_block[W2index9], leading_zeros_W2);

	input_block[W1index0] = Normalization(input_block[W1index0], leading_zeros_W1);
	input_block[W1index1] = Normalization(input_block[W1index1], leading_zeros_W1);
	input_block[W1index2] = Normalization(input_block[W1index2], leading_zeros_W1);
	input_block[W1index3] = Normalization(input_block[W1index3], leading_zeros_W1);
	input_block[W1index4] = Normalization(input_block[W1index4], leading_zeros_W1);

    input_block[W0index0] = Normalization(input_block[W0index0], leading_zeros_W0);
	input_block[W0index1] = Normalization(input_block[W0index1], leading_zeros_W0);
	input_block[W0index2] = Normalization(input_block[W0index2], leading_zeros_W0);

	input_block[V0index0] = Normalization(input_block[V0index0], leading_zeros_V0);
	input_block[V0index1] = Normalization(input_block[V0index1], leading_zeros_V0);

	if (!BIT_GROUP_ALLOCATION) {
        allocate_bits(bit_reservoir, estimator_value_transmit, allocate_bits_transmit);
	}
	else {
	//-----GROUP BIT ALLOCATION-----
        bit_group_allocation(bit_reservoir, estimator_value_transmit, allocate_bits_transmit);
	}

	if (DITHER) {
	    Dither_TPDF(&input_block[W2index0], allocate_bits_transmit[3],3);
	    Dither_TPDF(&input_block[W2index1], allocate_bits_transmit[3],3);
	    Dither_TPDF(&input_block[W2index2], allocate_bits_transmit[3],3);
	    Dither_TPDF(&input_block[W2index3], allocate_bits_transmit[3],3);
	    Dither_TPDF(&input_block[W2index4], allocate_bits_transmit[3],3);
	    Dither_TPDF(&input_block[W2index5], allocate_bits_transmit[3],3);
	    Dither_TPDF(&input_block[W2index6], allocate_bits_transmit[3],3);
	    Dither_TPDF(&input_block[W2index7], allocate_bits_transmit[3],3);
	    Dither_TPDF(&input_block[W2index8], allocate_bits_transmit[3],3);
	    Dither_TPDF(&input_block[W2index9], allocate_bits_transmit[3],3);
        Dither_TPDF(&input_block[W1index0], allocate_bits_transmit[2],2);
	    Dither_TPDF(&input_block[W1index1], allocate_bits_transmit[2],2);
	    Dither_TPDF(&input_block[W1index2], allocate_bits_transmit[2],2);
	    Dither_TPDF(&input_block[W1index3], allocate_bits_transmit[2],2);
	    Dither_TPDF(&input_block[W1index4], allocate_bits_transmit[2],2);

	    Dither_TPDF(&input_block[W0index0], allocate_bits_transmit[1],1);
	    Dither_TPDF(&input_block[W0index1], allocate_bits_transmit[1],1);
	    Dither_TPDF(&input_block[W0index2], allocate_bits_transmit[1],1);

        Dither_TPDF(&input_block[V0index0], allocate_bits_transmit[0],0);
	    Dither_TPDF(&input_block[V0index1], allocate_bits_transmit[0],0);
    }

    if (QUANTIZER) {
        Quantization(&input_block[W2index0], allocate_bits_transmit[3]);
        Quantization(&input_block[W2index1], allocate_bits_transmit[3]);
        Quantization(&input_block[W2index2], allocate_bits_transmit[3]);
        Quantization(&input_block[W2index3], allocate_bits_transmit[3]);
        Quantization(&input_block[W2index4], allocate_bits_transmit[3]);
        Quantization(&input_block[W2index5], allocate_bits_transmit[3]);
        Quantization(&input_block[W2index6], allocate_bits_transmit[3]);
        Quantization(&input_block[W2index7], allocate_bits_transmit[3]);
        Quantization(&input_block[W2index8], allocate_bits_transmit[3]);
        Quantization(&input_block[W2index9], allocate_bits_transmit[3]);

        Quantization(&input_block[W1index0], allocate_bits_transmit[2]);
        Quantization(&input_block[W1index1], allocate_bits_transmit[2]);
        Quantization(&input_block[W1index2], allocate_bits_transmit[2]);
        Quantization(&input_block[W1index3], allocate_bits_transmit[2]);
        Quantization(&input_block[W1index4], allocate_bits_transmit[2]);

        Quantization(&input_block[W0index0], allocate_bits_transmit[1]);
        Quantization(&input_block[W0index1], allocate_bits_transmit[1]);
        Quantization(&input_block[W0index2], allocate_bits_transmit[1]);

        Quantization(&input_block[V0index0], allocate_bits_transmit[0]);
        Quantization(&input_block[V0index1], allocate_bits_transmit[0]);
    }

    if (OFFSET) {
		Offset(&input_block[W2index0], allocate_bits_transmit[3]);
		Offset(&input_block[W2index1], allocate_bits_transmit[3]);
		Offset(&input_block[W2index2], allocate_bits_transmit[3]);
		Offset(&input_block[W2index3], allocate_bits_transmit[3]);
		Offset(&input_block[W2index4], allocate_bits_transmit[3]);
		Offset(&input_block[W2index5], allocate_bits_transmit[3]);
		Offset(&input_block[W2index6], allocate_bits_transmit[3]);
		Offset(&input_block[W2index7], allocate_bits_transmit[3]);
		Offset(&input_block[W2index8], allocate_bits_transmit[3]);
		Offset(&input_block[W2index9], allocate_bits_transmit[3]);

		Offset(&input_block[W1index0], allocate_bits_transmit[2]);
		Offset(&input_block[W1index1], allocate_bits_transmit[2]);
		Offset(&input_block[W1index2], allocate_bits_transmit[2]);
		Offset(&input_block[W1index3], allocate_bits_transmit[2]);
		Offset(&input_block[W1index4], allocate_bits_transmit[2]);

		Offset(&input_block[W0index0], allocate_bits_transmit[1]);
		Offset(&input_block[W0index1], allocate_bits_transmit[1]);
		Offset(&input_block[W0index2], allocate_bits_transmit[1]);

		Offset(&input_block[V0index0], allocate_bits_transmit[0]);
		Offset(&input_block[V0index1], allocate_bits_transmit[0]);
    }



	// Audio coding ends here

    if (!packetizing) {

    	for (i = 0; i < BLOCKSIZE; i++) {
			output_block[i] = input_block[i];
		}
		for (i = 0; i < 4; i++) {
        	estimator_value_receive[i] = estimator_value_transmit[i];
		}
	}

	else {  // Packetizing algorithm starts here
    // Packetizing

        for (i = 0; i < 3; i++) {			// the whole data structure shall be initialized by zero in advance
            mydata_transmit.myword[i] = 0;
        }

	// First we write the estimator values (variances) to the first unsigned integer 64-bit word of the data union

        mydata_transmit.myword[0] += (unsigned int)estimator_value_transmit[0];		// add the first estimator value to the first word

        for (i = 1; i < 4; i++) {
            mydata_transmit.myword[0] = mydata_transmit.myword[0] << 5;				// before adding shift left to insert the next estimator value
            mydata_transmit.myword[0] += (unsigned int)estimator_value_transmit[i];	// add the next estimator value to the first word
        }

	// Now we write the wavelet coefficients for the lowest subband V0 to the unsigned integer 64-bit words of the data union

        for (k = 0; k < 2; k++) {
            mydata_transmit.myword[2] =  mydata_transmit.myword[2] << allocate_bits_transmit[0]; 			// shift the data left by the number of allocated bits

            for (i = 2; i > 0; i--) {
                temp_data_transmit = mydata_transmit.myword[i-1] >> (64 - allocate_bits_transmit[0]); 		// "rescue" the data
                mydata_transmit.myword[i-1] =  mydata_transmit.myword[i-1] << allocate_bits_transmit[0];	// shift the data left by the number of allocated bits
                mydata_transmit.myword[i] += temp_data_transmit;
            }
            mydata_transmit.myword[0]   += (unsigned int)input_block[V0index0 + k*8] >> (32 - allocate_bits_transmit[0]); 	// now add the right boundary shifted data
        }

	// Now we write the wavelet coefficients for the next higher subband W0 to the unsigned integer 64-bit words of the data union

        for (k = 0; k < 3; k++) {
            mydata_transmit.myword[2] =  mydata_transmit.myword[2] << allocate_bits_transmit[1]; 			// shift the data left by the number of allocated bits

            for (i = 2; i > 0; i--) {
                temp_data_transmit = mydata_transmit.myword[i-1] >> (64 - allocate_bits_transmit[1]); 		// "rescue" the data
                mydata_transmit.myword[i-1] =  mydata_transmit.myword[i-1] << allocate_bits_transmit[1];	// shift the data left by the number of allocated bits
                mydata_transmit.myword[i] += temp_data_transmit;
            }
            mydata_transmit.myword[0]   += (unsigned int)input_block[W0index0 + k*8] >> (32 - allocate_bits_transmit[1]); 	// now add the right boundary shifted data
        }

	// Now we write the wavelet coefficients for the next higher subband W1 to the unsigned integer 64-bit words of the data union

        for (k = 0; k < 5; k++) {
            mydata_transmit.myword[2] =  mydata_transmit.myword[2] << allocate_bits_transmit[2]; 			// shift the data left by the number of allocated bits

            for (i = 2; i > 0; i--) {
                temp_data_transmit = mydata_transmit.myword[i-1] >> (64 - allocate_bits_transmit[2]); 		// "rescue" the data
                mydata_transmit.myword[i-1] =  mydata_transmit.myword[i-1] << allocate_bits_transmit[2];	// shift the data left by the number of allocated bits
                mydata_transmit.myword[i] += temp_data_transmit;
            }
            mydata_transmit.myword[0]   += (unsigned int)input_block[W1index0 + k*4] >> (32 - allocate_bits_transmit[2]); 	// now add the right boundary shifted data
        }

	// Now we write the wavelet coefficients for the highest subband W2 to the unsigned integer 64-bit words of the data union

        for (k = 0; k < 10; k++) {
            mydata_transmit.myword[2] =  mydata_transmit.myword[2] << allocate_bits_transmit[3]; 			// shift the data left by the number of allocated bits

            for (i = 2; i > 0; i--) {
                temp_data_transmit = mydata_transmit.myword[i-1] >> (64 - allocate_bits_transmit[3]); 		// "rescue" the data
                mydata_transmit.myword[i-1] =  mydata_transmit.myword[i-1] << allocate_bits_transmit[3];	// shift the data left by the number of allocated bits
                mydata_transmit.myword[i] += temp_data_transmit;
            }
            mydata_transmit.myword[0]   += (unsigned int)input_block[W2index0 + k*2] >> (32 - allocate_bits_transmit[3]); 	// now add the right boundary shifted data
        }

   // Bit stuffing: shift transmit data to left by one bit if only 147 bits were allocated.

        if (1 == lostbit) {
        	for (i = 2; i > 0; i--) {
        		mydata_transmit.myword[i] =  mydata_transmit.myword[i] << 1; // shift the data left by the number of allocated bits
        		temp_data_transmit = mydata_transmit.myword[i-1] >> 63; 		 // "rescue" the data
        		mydata_transmit.myword[i]  += temp_data_transmit;
        	}
        }

//    WaveletCompression( &input_block[0], (data*)&mydata_transmit);

   // Transmission: Copy mydata_transmit to mydata_receive

        for (i = 0; i < 21; i++) {
            mydata_receive.mybyte[i] = mydata_transmit.mybyte[i];
        }

//    WaveletDecompression((data*)&mydata_receive, &output_block[0]);

	// Unpacketizing

    // First we read the estimator values (variances) to the 3rd unsigned integer 64-bit word of the data union

        estimator_value_receive[0] = (unsigned int)(((mydata_receive.myword[2] & 0xffffffffff) >> 35));
        estimator_value_receive[1] = (unsigned int)(((mydata_receive.myword[2] & 0x07ffffffff) >> 30));
        estimator_value_receive[2] = (unsigned int)(((mydata_receive.myword[2] & 0x003fffffff) >> 25));
        estimator_value_receive[3] = (unsigned int)(((mydata_receive.myword[2] & 0x0001ffffff) >> 20));

        if (!BIT_GROUP_ALLOCATION) {
            allocate_bits(bit_reservoir, estimator_value_receive, allocate_bits_receive);
        }

        else {
    //-----GROUP BIT ALLOCATION-----
            bit_group_allocation(bit_reservoir, estimator_value_receive, allocate_bits_receive);
        }

    // Next we align the wavelet coefficients in the unsigned integer 64-bit words of the data union

        for (i = 2; i > 0; i--) {
            mydata_receive.myword[i] = mydata_receive.myword[i] << 44; 	// left align the coefficient values in myword[2].
            temp_data_receive = (unsigned long long)mydata_receive.myword[i-1] >> 20;		// "rescue" for the bits to be lost when shifting left myword[1]
            mydata_receive.myword[i] += temp_data_receive;
        }
        mydata_receive.myword[0] = mydata_receive.myword[0] << 44; 		// left align the coefficient values in myword[0].

    // Next we read the wavelet coefficients V0 from the unsigned integer 64-bit words of the data union

        for (k = 0; k < 2; k++) {
            output_block[V0index0 + k*8] = (int)((((mydata_receive.myword[2]) >> 32) & 0xffffffff)); // Easier to understand and not less efficient to do this in two lines
            output_block[V0index0 + k*8] = output_block[V0index0 + k*8] & (int)(0xffffffff << (32 - allocate_bits_receive[0]));

            for (i = 2; i > 0; i--) {
                mydata_receive.myword[i]   =  mydata_receive.myword[i] << allocate_bits_receive[0]; 	// shift the data left by the number of allocated bits
                temp_data_receive = (unsigned long long)mydata_receive.myword[i-1] >> (64 - allocate_bits_receive[0]); 	// "rescue" the data
                mydata_receive.myword[i]   += temp_data_receive;
            }
            mydata_receive.myword[0]   =  mydata_receive.myword[0] << allocate_bits_receive[0]; 		// shift the data left by the number of allocated bits	}
        }

	// Next we read the wavelet coefficients W0 from the unsigned integer 64-bit words of the data union

        for (k = 0; k < 3; k++) {
            output_block[W0index0 + k*8] = (int)((((mydata_receive.myword[2]) >> 32) & 0xffffffff)); // Easier to understand and not less efficient to do this in two lines
            output_block[W0index0 + k*8] = output_block[W0index0 + k*8] & (int)(0xffffffff << (32 - allocate_bits_receive[1]));

            for (i = 2; i > 0; i--) {
                mydata_receive.myword[i]   =  mydata_receive.myword[i] << allocate_bits_receive[1]; 	// shift the data left by the number of allocated bits
                temp_data_receive = (unsigned long long)mydata_receive.myword[i-1] >> (64 - allocate_bits_receive[1]); 	// "rescue" the data
                mydata_receive.myword[i]   += temp_data_receive;
            }
            mydata_receive.myword[0]   =  mydata_receive.myword[0] << allocate_bits_receive[1]; 		// shift the data left by the number of allocated bits	}
        }

	// Next we read the wavelet coefficients W1 from the unsigned integer 64-bit words of the data union

        for (k = 0; k < 5; k++) {
            output_block[W1index0 + k*4] = (int)((((mydata_receive.myword[2]) >> 32) & 0xffffffff)); // Easier to understand and not less efficient to do this in two lines
            output_block[W1index0 + k*4] = output_block[W1index0 + k*4] & (int)(0xffffffff << (32 - allocate_bits_receive[2]));

            for (i = 2; i > 0; i--) {
                mydata_receive.myword[i]   =  mydata_receive.myword[i] << allocate_bits_receive[2]; 	// shift the data left by the number of allocated bits
                temp_data_receive = (unsigned long long)mydata_receive.myword[i-1] >> (64 - allocate_bits_receive[2]); 	// "rescue" the data
                mydata_receive.myword[i]   += temp_data_receive;
            }
            mydata_receive.myword[0]   =  mydata_receive.myword[0] << allocate_bits_receive[2]; 		// shift the data left by the number of allocated bits	}
        }

	// Next we read the wavelet coefficients W2 from the unsigned integer 64-bit words of the data union

        for (k = 0; k < 10; k++) {
            output_block[W2index0 + k*2] = (int)((((mydata_receive.myword[2]) >> 32) & 0xffffffff)); // Easier to understand and not less efficient to do this in two lines
            output_block[W2index0 + k*2] = output_block[W2index0 + k*2] & (int)(0xffffffff << (32 - allocate_bits_receive[3]));

            for (i = 2; i > 0; i--) {
                mydata_receive.myword[i]   =  mydata_receive.myword[i] << allocate_bits_receive[3]; 	// shift the data left by the number of allocated bits
                temp_data_receive = (unsigned long long)mydata_receive.myword[i-1] >> (64 - allocate_bits_receive[3]); 	// "rescue" the data
                mydata_receive.myword[i]   += temp_data_receive;
            }
            mydata_receive.myword[0]   =  mydata_receive.myword[0] << allocate_bits_receive[3]; 		// shift the data left by the number of allocated bits	}
        }
	}		// Packetizing algorithm ends here


	// Audio decoding starts here

	// Calculate estimator "history" - might not be not needed for block signal processing
	sigmaK_receive[3] = estimator(estimator_value_receive[3], sigmaK_receive[3], estimator_weight_receive[3]);
	sigmaK_receive[2] = estimator(estimator_value_receive[2], sigmaK_receive[2], estimator_weight_receive[2]);
    sigmaK_receive[1] = estimator(estimator_value_receive[1], sigmaK_receive[1], estimator_weight_receive[1]);
	sigmaK_receive[0] = estimator(estimator_value_receive[0], sigmaK_receive[0], estimator_weight_receive[0]);

    if (estimation) {
        W2 = 32 - sigmaK_receive[3];
        W1 = 32 - sigmaK_receive[2];
        W0 = 32 - sigmaK_receive[1];
        V0 = 32 - sigmaK_receive[0];
    }
    else {
        W2 = 32 - estimator_value_receive[3];
        W1 = 32 - estimator_value_receive[2];
        W0 = 32 - estimator_value_receive[1];
        V0 = 32 - estimator_value_receive[0];
    }

    InverseNorm(&output_block[W2index0], W2);	//InverseNorm with W2
	InverseNorm(&output_block[W2index1], W2);
	InverseNorm(&output_block[W2index2], W2);
	InverseNorm(&output_block[W2index3], W2);
	InverseNorm(&output_block[W2index4], W2);
	InverseNorm(&output_block[W2index5], W2);
	InverseNorm(&output_block[W2index6], W2);
	InverseNorm(&output_block[W2index7], W2);
	InverseNorm(&output_block[W2index8], W2);
	InverseNorm(&output_block[W2index9], W2);

	InverseNorm(&output_block[W1index0], W1);	//InverseNorm with W1
	InverseNorm(&output_block[W1index1], W1);
	InverseNorm(&output_block[W1index2], W1);
	InverseNorm(&output_block[W1index3], W1);
	InverseNorm(&output_block[W1index4], W1);

    InverseNorm(&output_block[W0index0], W0);	//InverseNorm with W0
	InverseNorm(&output_block[W0index1], W0);
	InverseNorm(&output_block[W0index2], W0);

    InverseNorm(&output_block[V0index0], V0);	//InverseNorm with V0
	InverseNorm(&output_block[V0index1], V0);

	// Audio decoding ends here

	// Third Level Reconstruction

	// inverser Updateschritt

	output_block[14] -= ((output_block[10]>>2) + (output_block[18]>>2));

	output_block[6]  -= ((output_block[2] >>2) + (output_block[10]>>2));

	// inverser Praediktionsschritt

	output_block[18] += output_block[14];		// Randangepasstes Wavelet fuer Prediktion

	output_block[10] += ((output_block[6]>>1) + (output_block[14]>>1));

	output_block[2]  += output_block[6];		// Randangepasstes Wavelet fuer Prediktion


	// Second Level Reconstruction

	// inverser Updateschritt

	output_block[18]  -= output_block[16]>>1;	// Randangepasstes Wavelet fuer Update

	for (i = 14; i >= 2; i-=4) {
		output_block[i] -= ((output_block[i-2]>>2) + (output_block[i+2]>>2));
	}

	// inverser Praediktionsschritt

	for (i = 16; i >= 4; i-=4) {
		output_block[i]   += ((output_block[i-2]>>1) + (output_block[i+2]>>1));
	}

	output_block[0]  += output_block[2];		// Randangepasstes Wavelet fuer inverse Prediktion

	// First Level Reconstruction

	// inverser Updateschritt

	for (i = 17; i >= 1; i-=2) {
		output_block[i+1] -= ((output_block[i]>>2) + (output_block[i+2]>>2));
	}

	output_block[0]  -= output_block[1] >>1;	// Randangepasstes Wavelet fuer inverses Update

	// inverser Praediktionsschritt

	output_block[19] += output_block[18];		// Randangepasstes Wavelet fuer inverse Prediktion

	for (i = 17; i >= 1; i-=2) {
		output_block[i]   += ((output_block[i-1]>>1) + (output_block[i+1]>>1));
	}






//    WaveletCompression( &input_block[0], (data*)&mydata_transmit);
//
//   // Transmission: Copy mydata_transmit to mydata_receive
//
//        for (i = 0; i < 21; i++) {
//            mydata_receive.mybyte[i] = mydata_transmit.mybyte[i];
//        }
//
//    WaveletDecompression((data*)&mydata_receive, &output_block[0]);

	#pragma vector_for
	for (k = 0; k < BUFFER_SIZE*2; k+=2) {
		SP0A_buffer[k] = SP0A_buffer[k+1] = output_block[k>>1];
	}
}

//-------------------------------------------------------------------------------------------------------

void main(void) {
    adi_initComponents();
    init_interrupts();
    configure_CODEC();
    init_SPORT_DMA();
    configure_SPORT();
    enable_SPORT();

    while (1) {
        // do something useful...
        idle();
    }
}

