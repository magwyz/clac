#ifndef CODEC_H
#define CODEC_H

const unsigned frameSizes[] = {512, 1024, 2048};
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
