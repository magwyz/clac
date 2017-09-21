
#include <math.h>

#include <distribution.h>


static inline float exp1(float x) {
  x = 1.f + x / 256.f;
  x *= x; x *= x; x *= x; x *= x;
  x *= x; x *= x; x *= x; x *= x;
  return x;
}


Distribution::Distribution(int i_codeMin, int i_codeMax, uint16_t b,
                           unsigned i_log2TotalFreq)
{
    const unsigned i_nbSymbols = i_codeMax - i_codeMin + 1;
    const unsigned i_totalFreq = 1 << i_log2TotalFreq;

    float pf_freqs[i_nbSymbols];

    const unsigned i_nbFreqToAssign = i_totalFreq - 1 * i_nbSymbols;

    float sum = 0;
    for (int i = i_codeMin; i <= i_codeMax; ++i)
    {
        float p = 1.f / (2.f * b) * exp1(-std::fabs(i) / b);
        pf_freqs[i - i_codeMin] = p;
        sum += p;
    }

    float alpha = i_nbFreqToAssign / sum;

    unsigned sum2 = 0;
    freqs = new uint32_t[i_nbSymbols];
    for (unsigned i = 0; i < i_nbSymbols; ++i)
    {
        freqs[i] = 1 + floorf(pf_freqs[i] * alpha);
        sum2 += freqs[i];
    }

    unsigned i_missingCount = i_totalFreq - sum2;
    for (unsigned i = 0; i < i_missingCount; ++i)
        freqs[i]++;
}


Distribution::~Distribution()
{
    delete[] freqs;
}


uint32_t *Distribution::getFreqs()
{
    return freqs;
}
