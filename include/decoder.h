#ifndef DECODER_H
#define DECODER_H

#include <vector>
#include <parcorgenerator.h>
#include <stdint.h>


extern "C" {
#include <rangeCoder/rangeCoder.h>
#include <rangeCoder/quasiStaticModel.h>
}


class Decoder
{
public:
    int decodeFrame(char *p_buffer, size_t &offset, std::vector<int16_t> &decodedFrameData);
    int16_t getNextPrediction(RangeCoder *rc, QuasiStaticModel *qsm,
                              const unsigned i_log2TotalFreq, const int i_codeMin, int i_test);

private:
    ParCorGenerator pcg;
};


#endif // DECODER_H
