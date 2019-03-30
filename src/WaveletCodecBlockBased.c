/*
 * WaveletCodecBlockBased.c
 *
 *  Created on: Feb 16, 2019
 *      Author: Hiep Truong
 */
/**************************************************************************************************/
/*      Includes                                                                                  */
/**************************************************************************************************/
#include "WaveletCodecBlockBased.h"
#include "Functions_32bit.h"

/**************************************************************************************************/
/*      Internal Defines and Macros                                                               */
/**************************************************************************************************/
#define DITHER     0
#define QUANTIZER  0
#define OFFSET     0

#define BIT_GROUP_ALLOCATION  1     // Switch between Greedy Bit Allocation (Ramstadt) and Bit Group Allocation
#define estimation  1               // Switch between gain estimation and block gain
#define lostbit     0                       // This flag is used for bit stuffing - we can avoid using the estimators here!

#define W2index0    1     // Index for 10 W2
#define W2index1    3     // Index for 10 W2
#define W2index2    5     // Index for 10 W2
#define W2index3    7     // Index for 10 W2
#define W2index4    9     // Index for 10 W2
#define W2index5   11     // Index for 10 W2
#define W2index6   13     // Index for 10 W2
#define W2index7   15     // Index for 10 W2
#define W2index8   17     // Index for 10 W2
#define W2index9   19     // Index for 10 W2

#define W1index0    0     // Index for 5 W1
#define W1index1    4     // Index for 5 W1
#define W1index2    8     // Index for 5 W1
#define W1index3    12    // Index for 5 W1
#define W1index4    16    // Index for 5 W1

#define W0index0    2     // Index for 3 W0
#define W0index1    10    // Index for 3 W0
#define W0index2    18    // Index for 3 W0

#define V0index0    6     // Index for 2 V0
#define V0index1    14    // Index for 2 V0

/**************************************************************************************************/
/*      Internal variables                                                                        */
/**************************************************************************************************/
static unsigned int estimator_weight_transmit[4] = {0, 0, 10, 0};
static unsigned int estimator_weight_receive[4]  = {0, 0, 0, 0};

static unsigned int max_W2, max_W1, max_W0, max_V0;
static unsigned int sigmaK_transmit[4] = {0, 0, 0, 0};
static unsigned int sigmaK_receive[4]  = {0, 0, 0, 0};


static int 	W2, W1, W0, V0; 			// Estimator in Synthesis process
static int 	leading_zeros_W2 = 0,
   		    leading_zeros_W1 = 0,
   		    leading_zeros_W0 = 0,
   		    leading_zeros_V0 = 0;

static unsigned int estimator_value_transmit[4];
static unsigned int estimator_value_receive[4] = {0, 0, 0, 0};

static unsigned int bit_reservoir = 148;		// ~7,4bit/sample = 355,2kbit/s
//static unsigned int bit_reservoir = 92;		// ~4,6bit/sample = 220,8kbit/s
//static unsigned int bit_reservoir = 64;		// ~3,2bit/sample = 153,6kbit/s
//static unsigned int bit_reservoir = 78;		// ~3,9bit/sample = 153,6kbit/s

static unsigned int allocate_bits_transmit[4] = {0, 0, 0, 0};
static unsigned int allocate_bits_receive[4] = {0, 0, 0, 0};

static unsigned long long int temp_data_transmit = 0;	// temporary "rescue" variable for data to be shifted (and maybe lost during shifting)
static unsigned long long int temp_data_receive = 0;	// temporary "rescue" variable for data to be shifted (and maybe lost during shifting)


/**************************************************************************************************/
/*      EXTERNAL FUNCTION IMPLEMENTATIONS                                                         */
/**************************************************************************************************/
//#if defined (MIC_TX_DEVICE)
void WaveletCompression( int* input_block, volatile data* mydata_transmit)
{
	int i, k;
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

	for (i = 0; i < 3; i++) {			// the whole data structure shall be initialized by zero in advance
		mydata_transmit->myword[i] = 0;
	}

	// First we write the estimator values (variances) to the first unsigned integer 64-bit word of the data union

	mydata_transmit->myword[0] += (unsigned int)estimator_value_transmit[0];		// add the first estimator value to the first word

	for (i = 1; i < 4; i++) {
		mydata_transmit->myword[0] = mydata_transmit->myword[0] << 5;				// before adding shift left to insert the next estimator value
		mydata_transmit->myword[0] += (unsigned int)estimator_value_transmit[i];	// add the next estimator value to the first word
	}

	// Now we write the wavelet coefficients for the lowest subband V0 to the unsigned integer 64-bit words of the data union

	for (k = 0; k < 2; k++) {
		mydata_transmit->myword[2] =  mydata_transmit->myword[2] << allocate_bits_transmit[0]; 			// shift the data left by the number of allocated bits

		for (i = 2; i > 0; i--) {
			temp_data_transmit = mydata_transmit->myword[i-1] >> (64 - allocate_bits_transmit[0]); 		// "rescue" the data
			mydata_transmit->myword[i-1] =  mydata_transmit->myword[i-1] << allocate_bits_transmit[0];	// shift the data left by the number of allocated bits
			mydata_transmit->myword[i] += temp_data_transmit;
		}
		mydata_transmit->myword[0]   += (unsigned int)input_block[V0index0 + k*8] >> (32 - allocate_bits_transmit[0]); 	// now add the right boundary shifted data
	}

	// Now we write the wavelet coefficients for the next higher subband W0 to the unsigned integer 64-bit words of the data union

	for (k = 0; k < 3; k++) {
		mydata_transmit->myword[2] =  mydata_transmit->myword[2] << allocate_bits_transmit[1]; 			// shift the data left by the number of allocated bits

		for (i = 2; i > 0; i--) {
			temp_data_transmit = mydata_transmit->myword[i-1] >> (64 - allocate_bits_transmit[1]); 		// "rescue" the data
			mydata_transmit->myword[i-1] =  mydata_transmit->myword[i-1] << allocate_bits_transmit[1];	// shift the data left by the number of allocated bits
			mydata_transmit->myword[i] += temp_data_transmit;
		}
		mydata_transmit->myword[0]   += (unsigned int)input_block[W0index0 + k*8] >> (32 - allocate_bits_transmit[1]); 	// now add the right boundary shifted data
	}

	// Now we write the wavelet coefficients for the next higher subband W1 to the unsigned integer 64-bit words of the data union

	for (k = 0; k < 5; k++) {
		mydata_transmit->myword[2] =  mydata_transmit->myword[2] << allocate_bits_transmit[2]; 			// shift the data left by the number of allocated bits

		for (i = 2; i > 0; i--) {
			temp_data_transmit = mydata_transmit->myword[i-1] >> (64 - allocate_bits_transmit[2]); 		// "rescue" the data
			mydata_transmit->myword[i-1] =  mydata_transmit->myword[i-1] << allocate_bits_transmit[2];	// shift the data left by the number of allocated bits
			mydata_transmit->myword[i] += temp_data_transmit;
		}
		mydata_transmit->myword[0] += (unsigned int)input_block[W1index0 + k*4] >> (32 - allocate_bits_transmit[2]); 	// now add the right boundary shifted data
	}

	// Now we write the wavelet coefficients for the highest subband W2 to the unsigned integer 64-bit words of the data union

	for (k = 0; k < 10; k++) {
		mydata_transmit->myword[2] = mydata_transmit->myword[2] << allocate_bits_transmit[3]; 			// shift the data left by the number of allocated bits

		for (i = 2; i > 0; i--) {
			temp_data_transmit = mydata_transmit->myword[i-1] >> (64 - allocate_bits_transmit[3]); 		// "rescue" the data
			mydata_transmit->myword[i-1] =  mydata_transmit->myword[i-1] << allocate_bits_transmit[3];	// shift the data left by the number of allocated bits
			mydata_transmit->myword[i] += temp_data_transmit;
		}
		mydata_transmit->myword[0]   += (unsigned int)input_block[W2index0 + k*2] >> (32 - allocate_bits_transmit[3]); 	// now add the right boundary shifted data
	}

	// Bit stuffing: shift transmit data to left by one bit if only 147 bits were allocated.

	if (1 == lostbit) {
		for (i = 2; i > 0; i--) {
			mydata_transmit->myword[i] =  mydata_transmit->myword[i] << 1; // shift the data left by the number of allocated bits
			temp_data_transmit = mydata_transmit->myword[i-1] >> 63; 		 // "rescue" the data
			mydata_transmit->myword[i]  += temp_data_transmit;
		}
	}
}

//#elif defined (MIC_RX_DEVICE)
void WaveletDecompression(data* mydata_receive, int* output_block)
{
	int i, k;
	// Unpacketizing

	// First we read the estimator values (variances) to the 3rd unsigned integer 64-bit word of the data union

	estimator_value_receive[0] = (unsigned int)(((mydata_receive->myword[2] & 0xffffffffff) >> 35));
	estimator_value_receive[1] = (unsigned int)(((mydata_receive->myword[2] & 0x07ffffffff) >> 30));
	estimator_value_receive[2] = (unsigned int)(((mydata_receive->myword[2] & 0x003fffffff) >> 25));
	estimator_value_receive[3] = (unsigned int)(((mydata_receive->myword[2] & 0x0001ffffff) >> 20));

	if (!BIT_GROUP_ALLOCATION) {
		allocate_bits(bit_reservoir, estimator_value_receive, allocate_bits_receive);
	}
	else
	{
		//-----GROUP BIT ALLOCATION-----
		bit_group_allocation(bit_reservoir, estimator_value_receive, allocate_bits_receive);
	}

	// Next we align the wavelet coefficients in the unsigned integer 64-bit words of the data union

	for (i = 2; i > 0; i--) {
		mydata_receive->myword[i] = mydata_receive->myword[i] << 44; 	// left align the coefficient values in myword[2].
		temp_data_receive = (unsigned long long)mydata_receive->myword[i-1] >> 20;		// "rescue" for the bits to be lost when shifting left myword[1]
		mydata_receive->myword[i] += temp_data_receive;
	}
	mydata_receive->myword[0] = mydata_receive->myword[0] << 44; 		// left align the coefficient values in myword[0].

	// Next we read the wavelet coefficients V0 from the unsigned integer 64-bit words of the data union

	for (k = 0; k < 2; k++) {
		output_block[V0index0 + k*8] = (int)((((mydata_receive->myword[2]) >> 32) & 0xffffffff)); // Easier to understand and not less efficient to do this in two lines
		output_block[V0index0 + k*8] = output_block[V0index0 + k*8] & (int)(0xffffffff << (32 - allocate_bits_receive[0]));

		for (i = 2; i > 0; i--) {
			mydata_receive->myword[i]   =  mydata_receive->myword[i] << allocate_bits_receive[0]; 	// shift the data left by the number of allocated bits
			temp_data_receive = (unsigned long long)mydata_receive->myword[i-1] >> (64 - allocate_bits_receive[0]); 	// "rescue" the data
			mydata_receive->myword[i]   += temp_data_receive;
		}
		mydata_receive->myword[0]   =  mydata_receive->myword[0] << allocate_bits_receive[0]; 		// shift the data left by the number of allocated bits	}
	}

	// Next we read the wavelet coefficients W0 from the unsigned integer 64-bit words of the data union

	for (k = 0; k < 3; k++) {
		output_block[W0index0 + k*8] = (int)((((mydata_receive->myword[2]) >> 32) & 0xffffffff)); // Easier to understand and not less efficient to do this in two lines
		output_block[W0index0 + k*8] = output_block[W0index0 + k*8] & (int)(0xffffffff << (32 - allocate_bits_receive[1]));

		for (i = 2; i > 0; i--) {
			mydata_receive->myword[i]   =  mydata_receive->myword[i] << allocate_bits_receive[1]; 	// shift the data left by the number of allocated bits
			temp_data_receive = (unsigned long long)mydata_receive->myword[i-1] >> (64 - allocate_bits_receive[1]); 	// "rescue" the data
			mydata_receive->myword[i]   += temp_data_receive;
		}
		mydata_receive->myword[0]   =  mydata_receive->myword[0] << allocate_bits_receive[1]; 		// shift the data left by the number of allocated bits	}
	}

	// Next we read the wavelet coefficients W1 from the unsigned integer 64-bit words of the data union

	for (k = 0; k < 5; k++) {
		output_block[W1index0 + k*4] = (int)((((mydata_receive->myword[2]) >> 32) & 0xffffffff)); // Easier to understand and not less efficient to do this in two lines
		output_block[W1index0 + k*4] = output_block[W1index0 + k*4] & (int)(0xffffffff << (32 - allocate_bits_receive[2]));

		for (i = 2; i > 0; i--) {
			mydata_receive->myword[i]   =  mydata_receive->myword[i] << allocate_bits_receive[2]; 	// shift the data left by the number of allocated bits
			temp_data_receive = (unsigned long long)mydata_receive->myword[i-1] >> (64 - allocate_bits_receive[2]); 	// "rescue" the data
			mydata_receive->myword[i]   += temp_data_receive;
		}
		mydata_receive->myword[0]   =  mydata_receive->myword[0] << allocate_bits_receive[2]; 		// shift the data left by the number of allocated bits	}
	}

	// Next we read the wavelet coefficients W2 from the unsigned integer 64-bit words of the data union

	for (k = 0; k < 10; k++) {
		output_block[W2index0 + k*2] = (int)((((mydata_receive->myword[2]) >> 32) & 0xffffffff)); // Easier to understand and not less efficient to do this in two lines
		output_block[W2index0 + k*2] = output_block[W2index0 + k*2] & (int)(0xffffffff << (32 - allocate_bits_receive[3]));

		for (i = 2; i > 0; i--) {
			mydata_receive->myword[i]   =  mydata_receive->myword[i] << allocate_bits_receive[3]; 	// shift the data left by the number of allocated bits
			temp_data_receive = (unsigned long long)mydata_receive->myword[i-1] >> (64 - allocate_bits_receive[3]); 	// "rescue" the data
			mydata_receive->myword[i]   += temp_data_receive;
		}
		mydata_receive->myword[0]   =  mydata_receive->myword[0] << allocate_bits_receive[3]; 		// shift the data left by the number of allocated bits	}
	}
	// Packetizing algorithm ends here


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
}
//#endif
