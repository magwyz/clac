#ifndef ENCODER_H
#define ENCODER_H

#include <sys/types.h>

#include <vector>
#include <unordered_map>

extern "C" {
#include <rangeCoder/rangeCoder.h>
#include <rangeCoder/quasiStaticModel.h>
}

#define MAX_FRAME_SIZE (100 * 1024)

#define NB_QUANT_BITS 10
#define QUANT_HALF_INTERVAL 1.f


class Encoder
{
public:
    int encodeFrame(std::vector<int16_t> &data, unsigned i_dataOffset, unsigned &i_selectedFrameSize,
                    char *p_buffer, unsigned &i_size, bool b_refFrame);

private:
    int getFrameCompressedData(std::vector<int16_t> &data, unsigned i_dataOffset, unsigned &i_selectedFrameSize,
                               char *p_buffer, unsigned &i_size, bool b_refFrame);

    void calculateLPCcoeffs(std::vector<int16_t> &autoc, unsigned i_dataOffset, unsigned i_dataSize,
                            std::vector<float> &coeffs, std::vector<unsigned> &parCor);
    uint16_t calculateBParameter(std::vector<int16_t> codes);
    void buildRangeCoderDistribution(int i_codeMin, int i_codeMax, uint32_t freqs[], uint16_t b,
                                     unsigned i_log2TotalFreq);

    unsigned findBestShift(std::vector<int16_t> &data, unsigned i_dataOffset, unsigned i_dataSize);
    void selectBestParameters(std::vector<int16_t> &data, unsigned i_dataOffset, unsigned &i_selectedFrameSize,
                              std::vector<int16_t> &predictions, std::vector<unsigned> &ParCor,
                              bool b_refFrame);
    unsigned estimateFrameSize(unsigned i_predictionOrder, unsigned i_frameSize, u_int32_t *freqs,
                               unsigned i_nbSymbols, unsigned i_log2TotalFreq);
    float computeShanonEntropy(u_int32_t *freqs, unsigned i_nbSymbols, unsigned i_log2TotalFreq);

    unsigned quantizeParCor(float parcor, unsigned i_order);
    float getParCor(int quantizedParCor, unsigned i_order);
};

#endif // ENCODER_H