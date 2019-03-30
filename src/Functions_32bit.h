#include <stdlib.h> 		//for using: int abs(int _i);

//--------------------------------------------------------------------------//
// Prototypes																//
//--------------------------------------------------------------------------//

//************************ FUNCTION DECLARATION ***************************//
int rand_noise(unsigned int index);
int Count_Leading_Zeroes(int x1);
unsigned int max10(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j);
unsigned int max5(int a, int b, int c, int d, int e);
unsigned int max3(int a, int b, int c);
unsigned int max2(int a, int b);
int Normalization(int x, unsigned int m);
void InverseNorm(int *Norm, unsigned int m);
void Dither_TPDF(int *x, unsigned int AllocateBits, unsigned int subband);
void Quantization(int *x, unsigned int AllocateBits);
void Offset(int *x, unsigned int AllocateBits);
unsigned int estimator (unsigned int coef, unsigned int prev_value, unsigned int shift_value);
void allocate_bits(unsigned int number_of_bits, unsigned int* estimatorValue, unsigned int* AllocateBits);
void bit_group_allocation(unsigned int number_of_bits, unsigned int* estimatorValue, unsigned int* AllocateBits);
unsigned int maxIndex(int* estimatorValue);
