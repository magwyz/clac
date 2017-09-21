#ifndef CODEC_H
#define CODEC_H

const unsigned frameSizes[] = {512, 768, 896, 1024, 1152, 1280, 1408, 1536, 1664, 1792, 1920, 2048};
const unsigned i_nbFrameSizes = sizeof(frameSizes) / sizeof(*frameSizes);


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
