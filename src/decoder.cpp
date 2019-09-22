
#include <iostream>

#include <decoder.h>
#include <parcorgenerator.h>
#include <codec.h>
#include <distribution.h>
#include <mantissaexponent.h>


int Decoder::decodeFrame(char *p_buffer, size_t &offset, std::vector<int16_t> &decodedFrameData)
{
    RangeCoder* rc = (RangeCoder*)malloc(sizeof(RangeCoder));

    startDecoding(rc, (uint8_t *)p_buffer + offset);

    // Read the frame header.
    uint16_t header = readBits(rc, 16);

    if (header != 0b1111111111111110)
    {
        std::cout << "Wrong frame header." << std::endl;
        return 1;
    }

    const unsigned b_mantissa = readBits(rc, NB_BITS_B_MANTISSA);
    const unsigned b_exponent = readBits(rc, NB_BITS_B_EXPONENT);
    int16_t b = mantissaExponentToNumber(b_mantissa, b_exponent);

    const unsigned codeMin_mantissa = readBits(rc, NB_BITS_CODE_MIN_MAX_MANTISSA);
    const unsigned codeMin_exponent = readBits(rc, NB_BITS_CODE_MIN_MAX_EXPONENT);
    const unsigned codeMax_mantissa = readBits(rc, NB_BITS_CODE_MIN_MAX_MANTISSA);
    const unsigned codeMax_exponent = readBits(rc, NB_BITS_CODE_MIN_MAX_EXPONENT);

    const int64_t i_codeMin = -(int64_t)mantissaExponentToNumber(codeMin_mantissa + 1, codeMin_exponent);
    const int64_t i_codeMax = mantissaExponentToNumber(codeMax_mantissa + 1, codeMax_exponent);
    const unsigned i_nbSymbols = i_codeMax - i_codeMin + 1;

    // Read the predictor order.
    unsigned parCorSize = readBits(rc, 4) + 1;

    unsigned i_quant = NB_QUANT_BITS;

    std::vector<unsigned> parCor(parCorSize);
            static int i_test = 0;
    for (unsigned i = 0; i < parCorSize; ++i)
        parCor[i] = readBits(rc, i_quant);

    // Get the frame size
    unsigned i_frameSize;
    unsigned customFrameSize = readBits(rc, 1);
    if (customFrameSize)
        i_frameSize = readBits(rc, 11);
    else
        i_frameSize = frameSizes[readBits(rc, 2)];

    size_t firstDecodedValId = decodedFrameData.size();

    // Get the ref frame flag
    unsigned refFrame = readBits(rc, 1);
    if (refFrame)
    {
        int16_t firstSample = (int64_t)readBits(rc, 16) - (1 << 15);
        decodedFrameData.push_back(firstSample);
    }

    // Build the distribution
    const unsigned i_log2TotalFreq = 19;
    Distribution dist(i_codeMin, i_codeMax, b, i_log2TotalFreq);
    uint32_t *freqs = dist.getFreqs();

    // Get the coefs
    std::vector<float> coeffs;
    pcg.getLPCcoeffsFromParCor(parCor, coeffs, i_quant);

    QuasiStaticModel* qsm = createQuasiStaticModel(i_nbSymbols, i_log2TotalFreq,
                                 1 << (i_log2TotalFreq - 1), freqs);

    if (refFrame)
    {
        if (i_frameSize > 1)
        {
            int16_t predErr = getNextPrediction(rc, qsm, i_log2TotalFreq, i_codeMin, i_test);
            int16_t pred = decodedFrameData[firstDecodedValId];
            int16_t val = pred + predErr;
            decodedFrameData.push_back(val);
        }

        for (unsigned i = 2; i < i_frameSize && i < coeffs.size(); ++i)
        {
            int16_t predErr = getNextPrediction(rc, qsm, i_log2TotalFreq, i_codeMin, i_test);
            int16_t pred = 2 * decodedFrameData[firstDecodedValId + i - 1] - decodedFrameData[firstDecodedValId + i - 2];
            int16_t val = pred + predErr;
            decodedFrameData.push_back(val);
        }
    }


    unsigned i_startLPCPred = refFrame ? coeffs.size() : 0;

    for (unsigned i = i_startLPCPred; i < i_frameSize; ++i)
    {
        if (i_test == 1264)
            std::cout << i << " ";

        int16_t predErr = getNextPrediction(rc, qsm, i_log2TotalFreq, i_codeMin, i_test);

        int16_t pred = 0;
        for (unsigned j = 0; j < coeffs.size(); ++j)
            pred -= coeffs[j] * decodedFrameData[firstDecodedValId + i - 1 - j];

        int16_t val = pred + predErr;
        decodedFrameData.push_back(val);
    }

    size_t i_size = stopDecoding(rc);
    offset += i_size;

    return 0;
}


int16_t Decoder::getNextPrediction(RangeCoder *rc, QuasiStaticModel* qsm,
                                   const unsigned i_log2TotalFreq, const int i_codeMin, int i_test)
{
    uint64_t i_cumFreqDec = decodeCumFreq(rc, i_log2TotalFreq);

    unsigned i_sym = getSymbolFromCumFreqQSM(qsm, i_cumFreqDec);
    unsigned i_freq, i_cumFreq;
    unsigned i_freq2, i_cumFreq2;
    getSymbolFrequenciesQSM(qsm, i_sym, &i_freq, &i_cumFreq);
    getSymbolFrequenciesQSM(qsm, i_sym + 1, &i_freq2, &i_cumFreq2);

    updateAfterDecoding(rc, i_log2TotalFreq, i_freq, i_cumFreq);

    return i_sym + i_codeMin;
}
