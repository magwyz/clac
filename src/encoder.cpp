
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <future>
#include <cstring>
#include <map>

#include <encoder.h>
#include <codec.h>
#include <distribution.h>
#include <mantissaexponent.h>


Encoder::Encoder()
{
    // Add apodization functions

    apodizations.emplace_back(new BrickWall());
    apodizations.emplace_back(new Tuckey(0.1f));

#if 0
    {
        float overlap = 0.1f;
        float overlap_units = 1.0f/(1.0f - overlap) - 1.0f;
        float tukey_p = 0.2f;
        unsigned tukey_parts = 2;

        for (unsigned m = 0; m < tukey_parts; m++){
            float start = m / (tukey_parts + overlap_units);
            float end = (m + 1 + overlap_units) / (tukey_parts + overlap_units);
            apodizations.emplace_back(new PartialTuckey(tukey_p, start, end));
        }
    }

    {
        float overlap = 0.2f;
        float overlap_units = 1.0f/(1.0f - overlap) - 1.0f;
        float tukey_p = 0.2f;
         unsigned tukey_parts = 3;

        for (unsigned m = 0; m < tukey_parts; m++){
            float start = m / (tukey_parts + overlap_units);
            float end = (m + 1 + overlap_units) / (tukey_parts + overlap_units);
            apodizations.emplace_back(new PunchoutTukey(tukey_p, start, end));
        }
    }
#endif
}


int Encoder::encodeFrame(std::vector<int16_t> &data, unsigned i_dataOffset,
                         unsigned &i_selectedFrameSize, unsigned i_selectedQuantization,
                         char *p_buffer, unsigned &i_size, bool b_refFrame)
{

    getFrameCompressedData(data, i_dataOffset,
                           i_selectedFrameSize, i_selectedQuantization,
                           p_buffer, i_size, b_refFrame);

    float compressionRate = (float)i_size / (i_selectedFrameSize * 2);
    std::cout << "Compression: " << compressionRate << std::endl;

    return 0;
}


int Encoder::getFrameCompressedData(std::vector<int16_t> &data, unsigned i_dataOffset,
                                    unsigned &i_selectedFrameSize, unsigned i_selectedQuantization,
                                    char *p_buffer, unsigned &i_size, bool b_refFrame)
{
    std::vector<int16_t> codes;
    std::vector<unsigned> parCor;

    selectBestParameters(data, i_dataOffset,
                         i_selectedFrameSize, i_selectedQuantization,
                         codes, parCor, b_refFrame);


    // TODO: Get this data from the previous function.

    // Find the min and max.
    int64_t i_codeMin = *std::min_element(codes.begin(), codes.end());
    int64_t i_codeMax = *std::max_element(codes.begin(), codes.end());

    unsigned codeMin_mantissa, codeMin_exponent;
    unsigned codeMax_mantissa, codeMax_exponent;
    assert(i_codeMin <= 0);
    assert(i_codeMax > 0);
    numberToMantissaExponent(-i_codeMin, NB_BITS_CODE_MIN_MAX_MANTISSA, NB_BITS_CODE_MIN_MAX_EXPONENT,
                             codeMin_mantissa, codeMin_exponent);
    i_codeMin = -(int64_t)mantissaExponentToNumber(codeMin_mantissa + 1, codeMin_exponent);
    numberToMantissaExponent(i_codeMax, NB_BITS_CODE_MIN_MAX_MANTISSA, NB_BITS_CODE_MIN_MAX_EXPONENT,
                             codeMax_mantissa, codeMax_exponent);
    i_codeMax = mantissaExponentToNumber(codeMax_mantissa + 1, codeMax_exponent);

    const unsigned i_nbSymbols = i_codeMax - i_codeMin + 1;
    const unsigned i_log2TotalFreq = 19;

    int16_t b = calculateBParameter(codes);
    unsigned b_mantissa, b_exponent;
    b = numberToMantissaExponent(b, NB_BITS_B_MANTISSA, NB_BITS_B_EXPONENT,
                                 b_mantissa, b_exponent);

    //exportCodeDistribution(codes);

    Distribution dist(i_codeMin, i_codeMax, b, i_log2TotalFreq);
    uint32_t *freqs = dist.getFreqs();

    RangeCoder* rc = (RangeCoder*)malloc(sizeof(RangeCoder));
    QuasiStaticModel* qsm = createQuasiStaticModel(i_nbSymbols, i_log2TotalFreq,
                                 1 << (i_log2TotalFreq - 1), freqs);

    startEncoding(rc, (unsigned char*)p_buffer);

    // Write the frame header.
    writeBits(rc, 16, 65534);

    // Write the b parameter.
    writeBits(rc, NB_BITS_B_MANTISSA, b_mantissa);
    writeBits(rc, NB_BITS_B_EXPONENT, b_exponent);


    // Write the code min and max.
    writeBits(rc, NB_BITS_CODE_MIN_MAX_MANTISSA, codeMin_mantissa);
    writeBits(rc, NB_BITS_CODE_MIN_MAX_EXPONENT, codeMin_exponent);
    writeBits(rc, NB_BITS_CODE_MIN_MAX_MANTISSA, codeMax_mantissa);
    writeBits(rc, NB_BITS_CODE_MIN_MAX_EXPONENT, codeMax_exponent);

    // Write the predictor order.
    writeBits(rc, 4, parCor.size() - 1);

    // Write the predictor PARCOR.
    for (unsigned i = 0; i < parCor.size(); ++i)
        writeBits(rc, NB_QUANT_BITS, parCor[i]);

    // Write the frame size
    int i_frameSizeId = getFrameSizeId(i_selectedFrameSize);
    if (i_frameSizeId >= 0)
    {
        // Standard frame size.
        writeBits(rc, 1, 0);
        assert(i_frameSizeId < 4);
        writeBits(rc, 2, i_frameSizeId);
    }
    else
    {
        // Custom frame size.
        writeBits(rc, 1, 1);
        assert(i_selectedFrameSize < (1 << 12));
        writeBits(rc, 11, i_selectedFrameSize);
    }

    // Write the value of the first sample.
    if (b_refFrame)
    {
        writeBits(rc, 1, 1);
        writeBits(rc, 16, data[i_dataOffset] + (1 << 15));
    }
    else
        writeBits(rc, 1, 0);

    for (unsigned i = 0; i < codes.size(); ++i)
    {
        unsigned i_freq, i_cumFreq;

        const int32_t symb = codes[i] - i_codeMin;
        assert(symb >= 0);
        assert(symb < (int32_t)i_nbSymbols);
        getSymbolFrequenciesQSM(qsm, symb, &i_freq, &i_cumFreq);
        encodeFrequency(rc, i_log2TotalFreq, i_freq, i_cumFreq);
    }

    i_size = stopEncoding(rc);

    deleteQuasiStaticModel(qsm);
    free(rc);

    return 0;
}


uint16_t Encoder::calculateBParameter(std::vector<int16_t> codes)
{
    float b = 0;
    unsigned i_total = codes.size();
    for (unsigned i = 0; i < i_total; ++i)
        b += abs(codes[i]);

    return std::max((uint16_t)1, (uint16_t)roundf(b / i_total));
}


void Encoder::selectBestParameters(std::vector<int16_t> &data, unsigned i_dataOffset,
                                   unsigned &i_selectedFrameSize, unsigned &i_selectedQuantization,
                                   std::vector<int16_t> &predictions, std::vector<unsigned> &ParCor,
                                   bool b_refFrame)
{
    float i_lastSizePerSample = 0;
    unsigned i_lastOrder = 0;

    for (unsigned i = 0; i < i_nbFrameSizes; ++i)
    {
        unsigned i_frameSize = std::min((unsigned)data.size() - i_dataOffset, frameSizes[i]);

        for (unsigned order = 1; order <= 16; ++order)
        {

            for (unsigned i_quantIndex = 0;
                 i_quantIndex < sizeof(quantizations) / sizeof(*quantizations);
                 ++i_quantIndex)
            {
                for (auto apodization = apodizations.begin();
                     apodization != apodizations.end();
                     ++apodization)
                {
                    unsigned i_quant = quantizations[i_quantIndex];

                    std::vector<int16_t> newPredictions;

                    std::vector<float> newLPCCoeff(order);
                    std::vector<unsigned> newParCor(order);

                    if (!b_refFrame)
                        pcg.calculateLPCcoeffs(data, i_dataOffset - order,
                                               i_frameSize + order, newLPCCoeff,
                                               newParCor, i_quant, **apodization);
                    else
                        pcg.calculateLPCcoeffs(data, i_dataOffset,
                                               i_frameSize, newLPCCoeff,
                                               newParCor, i_quant, **apodization);

                    float error = 0;

                    if (b_refFrame)
                    {
                        if (i_frameSize > 1)
                        {
                            int16_t predErr = data[i_dataOffset + 1] - data[i_dataOffset];
                            newPredictions.push_back(predErr);
                            error += predErr * predErr;
                        }

                        for (unsigned i = 2; i < i_frameSize && i < newLPCCoeff.size(); ++i)
                        {
                            int16_t pred = 2 * data[i_dataOffset + i - 1] - data[i_dataOffset + i - 2];
                            int16_t predErr = data[i_dataOffset + i] - pred;
                            newPredictions.push_back(predErr);
                            error += predErr * predErr;
                        }
                    }

                    unsigned i_startLPCPred = b_refFrame ? order : 0;

                    for (unsigned i = i_startLPCPred; i < i_frameSize; ++i)
                    {
                        int16_t pred = 0;
                        for (unsigned j = 0; j < newLPCCoeff.size(); ++j)
                            pred -= newLPCCoeff[j] * data[i_dataOffset + i - 1 - j];

                        int16_t predErr = data[i_dataOffset + i] - pred;
                        newPredictions.push_back(predErr);
                        error += predErr * predErr;
                    }

                    error = 0.5f * std::log2(0.5f / i_frameSize * error) * i_frameSize / 8.f;
                    error += ceilf(order * NB_QUANT_BITS / 8.f);
                    error += 8;
                    error /= i_frameSize;

                    //if (i == 0 && order == 1 || i_sizePerSample < i_lastSizePerSample)
                    if (i == 0 && order == 1 || error < i_lastSizePerSample)
                    {
                        predictions = newPredictions;
                        ParCor = newParCor;
                        //i_lastSizePerSample = i_sizePerSample;
                        i_lastSizePerSample = error;
                        i_lastOrder = order;
                        i_selectedFrameSize = i_frameSize;
                        i_selectedQuantization = i_quant;
                    }
                }
            }
        }
    }

    std::cout << "Order: " << i_lastOrder
              << ", frame size: " << i_selectedFrameSize
              << ", quantization: " << i_selectedQuantization
              <<  ", size per sample: " << i_lastSizePerSample << std::endl;
}


void Encoder::exportCodeDistribution(std::vector<int16_t> &codes)
{
    // Distribution vizualization

    static unsigned dis_id = 0;

    std::ofstream ofsFreq;
    std::stringstream ssFreq;
    ssFreq << "distribution_" << dis_id << ".dat";

    std::map<int, unsigned> hist;
    for (unsigned i = 0; i < codes.size(); ++i)
        hist[codes[i]]++;

    ofsFreq.open(ssFreq.str());

    for (std::map<int, unsigned>::const_iterator it = hist.begin();
         it != hist.end(); ++it)
        ofsFreq << it->first  << "," << it->second << std::endl;

    ofsFreq.close();

    dis_id++;
}
