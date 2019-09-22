#ifndef CODEC_H
#define CODEC_H

//const unsigned frameSizes[] = {512, 768, 896, 1024, 1152, 1280, 1408, 1536, 1664, 1792, 1920, 2048};
const unsigned quantizations[] = {16};
const unsigned frameSizes[] = {512, 1024, 1536, 2048};
const unsigned i_nbFrameSizes = sizeof(frameSizes) / sizeof(*frameSizes);


#define NB_BITS_CODE_MIN_MAX_MANTISSA 4
#define NB_BITS_CODE_MIN_MAX_EXPONENT 4
#define NB_BITS_B_MANTISSA 4
#define NB_BITS_B_EXPONENT 4

static inline int getFrameSizeId(unsigned i_size)
{
    int ret = -1;
    for (unsigned i = 0; i < i_nbFrameSizes; ++i)
        if (i_size == frameSizes[i])
        {
            ret = i;
            break;
        }
    return ret;
}


#endif // CODEC_H
