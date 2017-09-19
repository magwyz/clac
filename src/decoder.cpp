
#include <iostream>

#include "decoder.h"


int Decoder::decodeFrame(char *p_buffer, size_t offset, std::vector<int16_t> decodedFrameData)
{
    RangeCoder* rc = (RangeCoder*)malloc(sizeof(RangeCoder));

    startDecoding(rc, (uint8_t *)p_buffer + offset);

    // Read the frame header.


    uint16_t header = readBits(rc, 16);

    if (header != 0b1111111111111110)
        return 1;

    int16_t b = readBits(rc, 15);

    int i_codeMin = (int64_t)readBits(rc, 15) - (1 << 14);
    int i_codeMax = (int64_t)readBits(rc, 15) - (1 << 14);

    // Read the predictor order.
    unsigned parCorSize = readBits(rc, 4) + 1;

    std::vector<unsigned> parCor(parCorSize);
    for (unsigned i = 0; i < parCorSize; ++i)
        parCor[i] = readBits(rc, 15) + (1 << 15);







    /*QuasiStaticModel* qsm = createQuasiStaticModel(i_nbSymbols, i_log2TotalFreq,
                                 1 << (i_log2TotalFreq - 1), freqs);*/
    return 0;
}
