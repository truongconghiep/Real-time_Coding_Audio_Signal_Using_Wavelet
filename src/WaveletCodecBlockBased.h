/*
 * WaveletCodecBlockBased.h
 *
 *  Created on: Feb 16, 2019
 *      Author: Hiep Truong
 */

#ifndef WAVELETCODECBLOCKBASED_H_
#define WAVELETCODECBLOCKBASED_H_
/**************************************************************************************************/
/*      Includes                                                                                  */
/**************************************************************************************************/

/**************************************************************************************************/
/*      Exported Defines                                                                          */
/**************************************************************************************************/
#define BUFFER_SIZE 20                  	// Size of buffer to be transmitted
#define BLOCKSIZE 20						// Size of buffer to be transmitted

/**************************************************************************************************/
/*      Exported typedef                                                                          */
/**************************************************************************************************/
typedef union {
    unsigned long long 	myword[3];	// here are 3 64-bit unsigned integers for 192bit
	unsigned char      	mybyte[21];	// here are 21 8-bit unsigned integers for 168bit
} data;

/**************************************************************************************************/
/*      Exported Functions                                                                        */
/**************************************************************************************************/

//#if defined (MIC_TX_DEVICE)
extern void WaveletCompression( int* input_block, volatile data* tx_data);
//#elif defined (MIC_RX_DEVICE)
extern void WaveletDecompression( data * mydata_receive, int* output_block);
//#endif

#endif /* WAVELETCODECBLOCKBASED_H_ */
