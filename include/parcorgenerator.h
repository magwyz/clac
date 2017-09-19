#ifndef PARCORGENERATOR_H
#define PARCORGENERATOR_H

#include <stdint.h>
#include <vector>


#define NB_QUANT_BITS 10
#define QUANT_HALF_INTERVAL 1.f


class ParCorGenerator
{
public:
    void calculateLPCcoeffs(std::vector<int16_t> &autoc, unsigned i_dataOffset, unsigned i_dataSize,
                            std::vector<float> &coeffs, std::vector<unsigned> &parCor);
    unsigned quantizeParCor(float parcor, unsigned i_order);
    float getParCor(int quantizedParCor, unsigned i_order);
};


#endif // PARCORGENERATOR_H
