#include "Functions_32bit.h"
#include <math.h>

unsigned int prand_seed[8] = {1,2,3,4,8,7,6,5};       // used in function prand()

extern unsigned int limit_reservoir;
extern unsigned int lostbit;

//******************************************************************************//
// 	Function:		rand_noise													//
// 	Parameters:		unsigned int index: choosing seed for the noise source		//
//	Return:			value of a random number									//
//	Description:	This function generates a random number, depended on the 	//
//					value of the index.											//
//******************************************************************************//
int rand_noise(unsigned int index) {
  unsigned int hi, lo;

  lo = 16807 * (prand_seed[index] & 0xFFFF);
  hi = 16807 * (prand_seed[index] >> 16);

  lo += (hi & 0x7FFF) << 16;
  lo += hi >> 15;

  if (lo > 0x7FFFFFFF) lo -= 0x7FFFFFFF;
  prand_seed[index] = (unsigned int)lo;

  return((prand_seed[index]) + 0x80000000); // -2^32
}

//******************************************************************************//
// 	Function:		Dither_TPDF													//
// 	Parameters:		int *x: subband coefficient                                 //
//					unsigned int AllocateBits: subband allocated bit			//
//					unsigned int subband: the number of a subband				//
//	Return:			None														//
//	Description:	Call function rand_noise(subband) to generate random noise 	//
//					Shift the noise right with a number of bit coresponding to	//
//					the number of allocated bits.								//
//					Adding the shifted noise to the audio signal				//	
//******************************************************************************//
void Dither_TPDF(int *x, unsigned int AllocateBits, unsigned int subband) {
	int noise;
	noise = rand_noise(subband);		// Generate noise
	noise = noise >> (AllocateBits);	// Shifting the noise right
	*x = (*x + noise) >> 1;				// Adding the noise to the audio signal
//	*x = (*x + noise);                     // Adding the noise to the audio signal
}

//******************************************************************************//
// 	Function:		Quantization												//
// 	Parameters:		int *x: subband coefficient                                 //
//					unsigned int AllocateBits: subband allocated bit			//
//	Return:			None														//
//	Description:	Quantizing subband coefficients								//
//******************************************************************************//
void Quantization(int *x, unsigned int AllocateBits) {
	int MaskBit = 0x80000000; //=0b1000 0000 0000 0000 0000 0000 0000 0000 = -2^32
    MaskBit= MaskBit >> (AllocateBits - 1);
	*x = *x & MaskBit;
}

//******************************************************************************//
// 	Function:		Offset														//
// 	Parameters:		int *x: subband coefficient								    //
//					unsigned int AllocateBits: subband allocated bit			//
//	Return:			None														//
//	Description:	Add offset before quantizing subband coefficients			//
//******************************************************************************//
void Offset(int *x, unsigned int AllocateBits) {
	int offset = 0x40000000; //=0b0100 0000 0000 0000 0000 0000 0000 0000 = 2^(32-1)
    offset= offset >> (AllocateBits-1);
	*x = *x + offset;
}

//******************************************************************************//
// 	Function:		Count_Leading_Zeroes                                        //
// 	Parameters:		int *x1: subband coefficient                                //
//	Return:			None														//
//	Description:	Counting number of leading zero of subband coefficients		//
//******************************************************************************//
int Count_Leading_Zeroes(int x1) {
	int m, n = 32;	//Bit size is 16
	int x = abs(x1) ; 	//counting leading zero
    if (x1 < -2147483647){		// limit the value to 2^32-1
        x = -2147483647;
    }
	unsigned int y;
	y = x >> 16; if (y != 0) { n = n - 16; x = y; }
	y = x >>  8; if (y != 0) { n = n -  8; x = y; }
	y = x >>  4; if (y != 0) { n = n -  4; x = y; }
	y = x >>  2; if (y != 0) { n = n -  2; x = y; }
	y = x >>  1; if (y != 0) m = n - 2;
				 else 		 m = n - 1;
	return (m - 1);	//except 1 bit zero for BitSign (MSB)
}

//******************************************************************************//
// 	Function:		Normalization												//
// 	Parameters:		int *x1: subband coefficient                                //
//					unsigned int m: number of leading zeros						//
//	Return:			None														//
//	Description:																//
//******************************************************************************//
int Normalization(int x, unsigned int m) {
	x = x << (m);	//shifting equal to number of leading zero except signed bit (MSB)
	return x;		
}

//******************************************************************************//
// 	Function:		InverseNorm													//
// 	Parameters:		int *Norm: subband coefficient                              //
//					unsigned int m: number of leading zeros						//
//	Return:			None														//
//	Description:																//
//******************************************************************************//
void InverseNorm(int *Norm, unsigned int m) {
	*Norm = *Norm >> (m);
}

//******************************************************************************//
// 	Function:		estimator													//
// 	Parameters:		unsigned int coef: subband coefficient                      //
//					unsigned int prev_value: previous estimator output			//
//                  unsigned int shift_value: estimator weighting factor        //
//	Return:			None														//
//	Discription:																//			
//******************************************************************************//
unsigned int estimator (unsigned int coef, unsigned int prev_value, unsigned int shift_value){
	unsigned int return_value;
	if (coef > prev_value) {
		return_value = coef;
	}
	else {
		return_value = (coef >> shift_value) + prev_value - (prev_value >> shift_value);
	}
	return return_value;
}
   
unsigned int max10(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j) {
	unsigned int max = abs(a);
	unsigned int b1  = abs(b);
	unsigned int c1  = abs(c);
	unsigned int d1  = abs(d);
	unsigned int e1  = abs(e);
	unsigned int f1  = abs(f);
	unsigned int g1  = abs(g);
	unsigned int h1  = abs(h);
	unsigned int i1  = abs(i);
	unsigned int j1  = abs(j);
    
    if (b1 > max) {max = b1;}
	if (c1 > max) {max = c1;}
	if (d1 > max) {max = d1;}
    if (e1 > max) {max = e1;}
    if (f1 > max) {max = f1;}
    if (g1 > max) {max = g1;}
    if (h1 > max) {max = h1;}
    if (i1 > max) {max = i1;}
	if (j1 > max) {max = j1;}
    
    return max;		
}

unsigned int max5(int a, int b, int c, int d, int e)	{
	unsigned int max = abs(a);
	unsigned int b1  = abs(b);
	unsigned int c1  = abs(c);
	unsigned int d1  = abs(d);
	unsigned int e1  = abs(e);
    
	if (b1 > max) {max = b1;}
	if (c1 > max) {max = c1;}
	if (d1 > max) {max = d1;}
    if (e1 > max) {max = e1;}
	
    return max;		
}

unsigned int max3(int a, int b, int c)	{
	unsigned int max = abs(a);
	unsigned int b1  = abs(b);
	unsigned int c1  = abs(c);
	if (b1 > max) {max = b1;}
	if (c1 > max) {max = c1;}
	return max;		
}

unsigned int max2(int a, int b)	{
	unsigned int max= abs(a);
	unsigned int b1 = abs(b);
	if (b1 > max) {max = b1;}	
	return max;	
}

void allocate_bits(unsigned int number_of_bits, unsigned int* estimator_value, unsigned int* allocate_bits) {
    unsigned int allocateIndex = 0;                   // Index to point to V0, W0, W1 or W2
    unsigned int bits_per_subband[4] = {2, 3, 5, 10}; // Bits to allocate to the subbands V0, W0, W1 and W2
    int variance[4];

    unsigned int i;
            
    for (i = 0; i < 4; i++) {
        variance[i] = estimator_value[i];
        allocate_bits[i] = 0;
    }
        
    while (number_of_bits > 0) {              // As long as bit reservoir is not yet emptied
        allocateIndex = maxIndex(variance);   // Find the subband with the highest variance and set the index to it
        variance[allocateIndex] -=  1;        // ...and reduce the variance by this one bit (-6dB)

        if (!limit_reservoir) {
        	number_of_bits -= bits_per_subband[allocateIndex];  // We take the according number of bits to assign
        	allocate_bits[allocateIndex]++;     				// Assign one bit to this subband...
        }

        else {	// if(limit_reservoir)
// Here we take care that the appropriate number of bits to assign is left in the bit reservoir
        	if ((allocateIndex == 3) && (number_of_bits > 9)) {		// Here we have enough bits to allocate to W2
        		number_of_bits -= bits_per_subband[allocateIndex];  // We take the according number of bits to assign
        		allocate_bits[allocateIndex]++;     				// Assign one bit to this subband W2...
        	}                                                    	// to the highest variance subband {1, 1, 2, 4}
        	else if ((allocateIndex == 2) && (number_of_bits > 4)) {// Here we have enough bits to allocate to W1
        		number_of_bits -= bits_per_subband[allocateIndex];  // We take the according number of bits to assign
        		allocate_bits[allocateIndex]++;     				// Assign one bit to the subband W1...
        	}
        	else if ((allocateIndex == 1) && (number_of_bits > 2)) {// Here we have enough bits to allocate to W0
        		number_of_bits -= bits_per_subband[allocateIndex];  // We take the according number of bits to assign
        		allocate_bits[allocateIndex]++;     				// Assign one bit to the subband W0...
        	}
        	else if ((allocateIndex == 0) && (number_of_bits > 1)) {// Here we have enough bits to allocate to V0
        		number_of_bits -= bits_per_subband[allocateIndex];  // We take the according number of bits to assign
        		allocate_bits[allocateIndex]++;     				// Assign one bit to the subband V0...
        	}

        	else {
        		switch (number_of_bits) {
        			case 9:
        				allocate_bits[2] += 1;	// assign 5 bits
        				allocate_bits[0] += 2;	// assign 4 bits
        				number_of_bits -= 9;
        				break;
        			case 8:
        				allocate_bits[2] += 1;	// assign 5 bits
        				allocate_bits[1] += 1;	// assign 3 bits
        				number_of_bits -= 8;
        				break;
        			case 7:
        				allocate_bits[2] += 1;	// assign 5 bits
        				allocate_bits[0] += 1;	// assign 2 bits
        				number_of_bits -= 7;
        				break;
        			case 6:
        				allocate_bits[1] += 2;								// Assign one bit to the subband W0...
        				number_of_bits -= 6;
        				break;
        			case 5:
        				allocate_bits[0] += 1;								// Assign one bit to the subband W0...
        				allocate_bits[1] += 1;								// Assign one bit to the subband W0...
        				number_of_bits -= 5;								// Here was a bug!
        				break;
        			case 4:
        				allocate_bits[0] += 2;								// Assign two bits to the subband V0...
        				number_of_bits -= 4;
        				break;
        			case 3:
        				allocate_bits[1] += 1;     							// Assign one bit to this subband W0...
        				number_of_bits -= 3;
        				break;
        			case 2:
        				allocate_bits[0] += 1;     							// Assign one bit to this subband V0...
        				number_of_bits -= 2;
        				break;
        			default:
        				number_of_bits -= 1;
        				break;
        		}
        	}
        }
    }
    return;
}

void bit_group_allocation(unsigned int number_of_bits, unsigned int* estimator_value, unsigned int* allocate_bits) {
     
    int  V0_W0_diff,
         W0_W1_diff,
         W1_W2_diff;
    
    int   bits2give, takeBits, totalBits, allocate_factor, allocate_reminder;

    lostbit = 0; // reset flag for bit allocation of 147bit only
    
    while (1){
    	totalBits = number_of_bits;
            
    	allocate_bits[3] = 0;
    	allocate_bits[2] = 0;
    	allocate_bits[1] = 0;
    	allocate_bits[0] = 0;
            
    	if (estimator_value[0] < estimator_value[1]) {
    		V0_W0_diff = 0;
    	}
    	else V0_W0_diff = estimator_value[0] - estimator_value[1];

    	if (estimator_value[1] < estimator_value[2]) {
    		W0_W1_diff = 0;
    	}
    	else W0_W1_diff = estimator_value[1] - estimator_value[2];

    	if (estimator_value[2] < estimator_value[3]) {
    		W1_W2_diff = 0;
    	}
    	else W1_W2_diff = estimator_value[2] - estimator_value[3];
            
    	bits2give = totalBits;
    	takeBits = V0_W0_diff << 1;					// Take two bits per allocated V0 bit
        
    	if (bits2give > takeBits) {
    		allocate_bits[0] += V0_W0_diff;
    	}                                           // Yes
    	else	{
    		allocate_bits[0] += bits2give >> 1; 	// Distribute the remaining bits to two V0 samples
    		break;                                  // No
        }                                               
        bits2give = bits2give - takeBits;
        takeBits  = (W0_W1_diff << 2) + W0_W1_diff;	// Take five bits per allocated W0 bit
        
        if (bits2give > takeBits) {	
            allocate_bits[1] += W0_W1_diff;         // Yes
            allocate_bits[0] += W0_W1_diff;
        }
        else {
        	allocate_factor = bits2give/5;
      	    allocate_bits[1] += allocate_factor;
            allocate_bits[0] += allocate_factor;
            break;                                  // No
        }  	

        bits2give = bits2give - takeBits;
        takeBits  = (W1_W2_diff << 3) + (W1_W2_diff << 1); // Take ten bits per allocated W1 bit
        
        if (bits2give > takeBits) {	
        	allocate_bits[2] += W1_W2_diff;
            allocate_bits[1] += W1_W2_diff;         // Yes
            allocate_bits[0] += W1_W2_diff;
        }
        else {
        	allocate_factor = bits2give/10;
            allocate_bits[2] += allocate_factor;
            allocate_bits[1] += allocate_factor;
            allocate_bits[0] += allocate_factor;
            break;                                  // No
        }  	

        bits2give = bits2give - takeBits;
        allocate_factor = bits2give/20;
        allocate_reminder = bits2give%20;
        allocate_bits[3] += allocate_factor;
        allocate_bits[2] += allocate_factor;
        allocate_bits[1] += allocate_factor;
        allocate_bits[0] += allocate_factor;

        //Modify Group Bit Allocation
        switch (allocate_reminder) {
    		case 19:
    			allocate_bits[3] += 1;	// assign 10 bits
    			allocate_bits[2] += 1;	// assign  5 bits
    			allocate_bits[0] += 2;	// assign  4 bits
    			break;
    		case 18:
    			allocate_bits[3] += 1;	// assign 10 bits
    			allocate_bits[2] += 1;	// assign  5 bits
    			allocate_bits[1] += 1;	// assign  3 bits
    			break;
    		case 17:
    			allocate_bits[3] += 1;	// assign 10 bits
    			allocate_bits[2] += 1;	// assign  5 bits
    			allocate_bits[0] += 1;	// assign  2 bits
    			break;
    		case 16:
    			allocate_bits[3] += 1;	// assign 10 bits
    			allocate_bits[1] += 2;	// assign  6 bits
    			break;
    		case 15:
    			allocate_bits[3] += 1;	// assign 10 bits
    			allocate_bits[1] += 1;	// assign  3 bits
    			allocate_bits[0] += 1;	// assign  2 bits
    			break;
    		case 14:
    			allocate_bits[3] += 1;	// assign 10 bits
    			allocate_bits[0] += 2;	// assign  4 bits
    			break;
    		case 13:
    			allocate_bits[3] += 1;	// assign 10 bits
    			allocate_bits[1] += 1;	// assign  3 bits
    			break;
    		case 12:
    			allocate_bits[3] += 1;	// assign 10 bits
    			allocate_bits[0] += 1;	// assign  2 bits
    			break;
    		case 11:
    			allocate_bits[2] += 1;	// assign  5 bits
    			allocate_bits[1] += 2;	// assign  6 bits
    			break;
    		case 10:
        		allocate_bits[2] += 1;	// assign 5 bits
        		allocate_bits[1] += 1;	// assign 3 bits
        		allocate_bits[0] += 1;	// assign 2 bits
        		break;
        	case 9:
        		allocate_bits[2] += 1;	// assign 5 bits
        		allocate_bits[0] += 2;	// assign 4 bits
        		break;
        	case 8:
        		allocate_bits[2] += 1;	// assign 5 bits
        		allocate_bits[1] += 1;	// assign 3 bits
        		break;
        	case 7:
        		allocate_bits[2] += 1;	// assign 5 bits
        		allocate_bits[0] += 1;	// assign 2 bits
        		break;
        	case 6:
        		allocate_bits[1] += 2;	// assign 6 bits
        		break;
        	case 5:
        		allocate_bits[1] += 1;	// assign 3 bits
        		allocate_bits[0] += 1;	// assign 2 bits
        		break;
        	case 4:
        		allocate_bits[0] += 2;	// assign 4 bits
        		break;
        	case 3:
        		allocate_bits[1] += 1;	// assign 3 bits
        		break;
        	case 2:
        		allocate_bits[0] += 1;	// assign 2 bits
        		break;
        	case 1:
        		lostbit = 1; 			// set flag for bit allocation of 147bit only
        		break;
        	default:
        		break;
        }
        break;
    } // end while(1)
    return;
}

unsigned int maxIndex(int* estimatorValue) {
    unsigned int max = estimatorValue[0]; // We assume that lowest subband has the highest signal variance...
    unsigned int returnIndex = 0;       // ...then we set the index to the highest variance subband to 0.

    if (estimatorValue[1] > max) {      // If subband W0 shows a higher signal variance than subband V0...
        max = estimatorValue[1];        // ... then W0 is the assumed highest variance subband
        returnIndex = 1;                // ... and we set the index to the highest variance subband to 1.
    }
    if (estimatorValue[2] > max) {      // If subband W1 shows a higher signal variance than the previously highest...
        max = estimatorValue[2];        // ... then W1 is the assumed highest variance subband
        returnIndex = 2;                // ... and we set the index to the highest variance subband to 2.
    }
    if (estimatorValue[3] > max) {      // If subband W2 shows a higher signal variance than the previously highest...
        max = estimatorValue[3];        // ... then W2 is the assumed highest variance subband
        returnIndex = 3;                // ... and we set the index to the highest variance subband to 3.
    }
    return returnIndex;                 // We return the index pointing to the highest variance subband
}
