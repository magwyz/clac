#ifndef RANGECODER_H
#define RANGECODER_H

#include <stdint.h>


#define MESSAGE_BIT_LENGTH 64 // The number of bits of the message variable.


typedef struct
{
    uint64_t i_message; // The current value of the message.
    uint64_t i_prodFreq; // The current product of the frequencies.

    unsigned char *p_data; // The data buffer.
    uint64_t i_dataOffset; // The offset where to read/write the current byte.

    unsigned char c_bitMask; // The bit mask to write one bit at a time in the buffer.
    unsigned int i_nbLeadingBitsSkipped; // The number of the remaining leading bits of the message to skip.

    unsigned int i_nbBitsShift; // The number of bits to shift the message after a safe product.
} RangeCoder;


/**
  * Init the range coder data structure.
  * @param rc a pointer on the range coder structure.
  * @parma p_data the pointer on the data buffer.
  */
void initRangeCoder(RangeCoder *rc, unsigned char *p_data);


/**
  * Start the encoding.
  * @param rc a pointer on the range coder structure.
  * @parma p_data the pointer on the data buffer.
  */
void startEncoding(RangeCoder *rc, unsigned char *p_data);


/**
  * Stop the encoding.
  * @param rc a pointer on the range coder structure.
  * @return the size of the compressed data.
  **/
unsigned stopEncoding(RangeCoder *rc);


/**
  * Encode a frequency.
  * @param rc a pointer on the range coder structure.
  * @param i_log2TotFreq the base 2 log of the total frequency.
  * @param i_symbolFreq the frequency of the symbol we want to encode.
  * @param i_symbolCumFreq the cumulative frequency of the symbol we want to encode.
  */
void encodeFrequency(RangeCoder *rc, unsigned i_log2TotFreq,
                     unsigned i_symbolFreq, uint64_t i_symbolCumFreq);


/**
  * Write in the data buffer a given number of the left bits of the message variable.
  * Shift left accordingly the variable.
  * Do not store the initial zeros of the message variable.
  * @param rc a pointer on the range coder structure.
  * @param i_nbBits the number of bits.
  */
void writeAndShiftBits(RangeCoder *rc, unsigned i_nbBits);


/**
  * Perform a product between two integers and shifts rigtht the results.
  * @param rc a pointer on the range coder structure.
  * @param x the first integer.
  * @param y the second integer.
  * @param i_mask the mask which determines the right shift.
  */
uint64_t safeProduct(RangeCoder *rc, uint64_t x, uint64_t y, uint64_t i_mask);


/**
  * Start the decoding.
  * @param rc a pointer on the range coder structure.
  * @param p_data the pointer on the data buffer.
  */
void startDecoding(RangeCoder *rc, unsigned char *p_data);


/**
  * Stop the decoding.
  * @return the number of bytes decoded.
  */
unsigned stopDecoding(RangeCoder *rc);


/**
  * Read in the data buffer a given number of bits.
  * Store them in the right of the message variable.
  * @param rc a pointer on the range coder structure.
  * @param i_nbBits the number of bits.
  */
void readAndShiftBits(RangeCoder *rc, unsigned i_nbBits);


/**
  * Decode the cumulative frequency of the next symbol.
  * @param rc a pointer on the range coder structure.
  * @param i_log2TotFreq the base 2 log of the total frequency.
  * @return the decoded cumulative frequency.
  **/
uint64_t decodeCumFreq(RangeCoder *rc, unsigned i_log2TotFreq);


/**
  * Update the range coder after the decoding of a frequency.
  * @param rc a pointer on the range coder structure.
  * @param i_log2TotFreq the base 2 log of the total frequency.
  * @param i_symbolFreq the frequency of the symbol that has just been decoded.
  * @param i_symbolCumFreq the cumulative frequency of the symbol that has just been decoded.
  */
void updateAfterDecoding(RangeCoder *rc, unsigned i_log2TotFreq,
                         unsigned i_symbolFreq, uint64_t i_symbolCumFreq);


/**
  * Write bits in the stream.
  * @param rc a pointer on the range coder structure.
  * @param i_nbBits the number of bits to write.
  * @param data the bits to write.
  */
void writeBits(RangeCoder *rc, unsigned i_nbBits, uint64_t data);


/**
  * Read bits in the stream.
  * @param rc a pointer on the range coder structure.
  * @param i_nbBits the number of bits to read.
  * @return the read data.
  */
uint64_t readBits(RangeCoder *rc, unsigned i_nbBits);

#endif
