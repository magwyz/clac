#ifndef PARCORGENERATOR_H
#define PARCORGENERATOR_H

#include <stdint.h>
#include <vector>
#include <unordered_map>

#include "apodization.h"


#define NB_QUANT_BITS 16
#define QUANT_HALF_INTERVAL 1.f


class ParCorGenerator
{
public:
    void calculateLPCcoeffs(std::vector<int16_t> &autoc, unsigned i_dataOffset, unsigned i_dataSize,
                            std::vector<float> &coeffs, std::vector<unsigned> &parCor,
                            unsigned i_quant, Apodization &apodization);
    void getLPCcoeffsFromParCor(std::vector<unsigned> &parCor, std::vector<float> &coeffs,
                                unsigned i_quant);
    unsigned quantizeParCor(float parcor, unsigned i_order, unsigned i_quant);
    float getParCor(int quantizedParCor, unsigned i_order, unsigned i_quant);

private:
    std::unordered_map<unsigned, std::vector<float> > windows;
};


#endif // PARCORGENERATOR_H
