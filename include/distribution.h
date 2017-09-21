#ifndef DISTRIBUTION_H
#define DISTRIBUTION_H

#include <stdint.h>


class Distribution
{
public:
    Distribution(int i_codeMin, int i_codeMax, uint16_t b,
                 unsigned i_log2TotalFreq);
    ~Distribution();
    uint32_t *getFreqs();
    void writeDistribution(unsigned disId);

private:
    uint32_t *freqs;

    int i_codeMin;
    int i_codeMax;

};


#endif // DISTRIBUTION_H
