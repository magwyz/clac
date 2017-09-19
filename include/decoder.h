#ifndef DECODER_H
#define DECODER_H

#include <vector>


extern "C" {
#include <rangeCoder/rangeCoder.h>
#include <rangeCoder/quasiStaticModel.h>
}


class Decoder
{
public:
    int decodeFrame(char *p_buffer, size_t offset, std::vector<int16_t> decodedFrameData);
};


#endif // DECODER_H
