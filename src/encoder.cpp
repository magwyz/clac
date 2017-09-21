
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <future>
#include <cstring>

#include <encoder.h>
#include <codec.h>
#include <distribution.h>


int Encoder::encodeFrame(std::vector<int16_t> &data, unsigned i_dataOffset, unsigned &i_selectedFrameSize,
                         char *p_buffer, unsigned &i_size, bool b_refFrame)
{

    getFrameCompressedData(data, i_dataOffset, i_selectedFrameSize, p_buffer, i_size, b_refFrame);

    float compressionRate = (float)i_size / (i_selectedFrameSize * 2);
    std::cout << "Compression: " << compressionRate << std::endl;
    if (compressionRate > 1)
        exit(1);

    return 0;
}


int Encoder::getFrameCompressedData(std::vector<int16_t> &data, unsigned i_dataOffset, unsigned &i_selectedFrameSize,
                                    char *p_buffer, unsigned &i_size, bool b_refFrame)
{
    std::vector<int16_t> codes;
    std::vector<unsigned> parCor;

    selectBestParameters(data, i_dataOffset, i_selectedFrameSize, codes, parCor, b_refFrame);


    // TODO: Get this data from the previous function.

    // Find the min and max.
    int64_t i_codeMin = *std::min_element(codes.begin(), codes.end());
    int64_t i_codeMax = *std::max_element(codes.begin(), codes.end());

    if (i_codeMin == i_codeMax) // We must have at least two symbols for the range coder model.
        i_codeMax = i_codeMin + 1;

    const unsigned i_nbSymbols = i_codeMax - i_codeMin + 1;
    const unsigned i_log2TotalFreq = 19;

    int16_t b = calculateBParameter(codes);

    Distribution dist(i_codeMin, i_codeMax, b, i_log2TotalFreq);
    uint32_t *freqs = dist.getFreqs();

    RangeCoder* rc = (RangeCoder*)malloc(sizeof(RangeCoder));
    QuasiStaticModel* qsm = createQuasiStaticModel(i_nbSymbols, i_log2TotalFreq,
                                 1 << (i_log2TotalFreq - 1), freqs);

    startEncoding(rc, (unsigned char*)p_buffer);

    // Write the frame header.
    writeBits(rc, 16, 65534);

    // Write the b parameter.
    assert(b < (1 << 13));
    writeBits(rc, 13, b);

    // Write the code min and max.
    assert(i_codeMin + (1 << 15) < (1 << 16));
    writeBits(rc, 16, i_codeMin + (1 << 15));
    assert(i_codeMax + (1 << 15) < (1 << 16));
    writeBits(rc, 16, i_codeMax + (1 << 15));

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
        assert(i_frameSizeId < 16);
        writeBits(rc, 4, i_frameSizeId);
    }
    else
    {
        // Custom frame size.
        writeBits(rc, 1, 0);
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


void Encoder::selectBestParameters(std::vector<int16_t> &data, unsigned i_dataOffset, unsigned &i_selectedFrameSize,
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
            std::vector<int16_t> newPredictions;

            std::vector<float> newLPCCoeff(order);
            std::vector<unsigned> newParCor(order);

            if (!b_refFrame)
                pcg.calculateLPCcoeffs(data, i_dataOffset - order, i_frameSize + order, newLPCCoeff, newParCor);
            else
                pcg.calculateLPCcoeffs(data, i_dataOffset, i_frameSize, newLPCCoeff, newParCor);

#if 0
            std::cout << "  LPC coefs" << std::endl;
            for (unsigned i = 0; i < newLPCCoeff.size(); ++i)
                std::cout << i << ": " << newLPCCoeff[i] << std::endl;
#endif

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
            }
        }
    }

    std::cout << "Order: " << i_lastOrder << ", frame size: " << i_selectedFrameSize <<  ", size per sample: " << i_lastSizePerSample << std::endl;
}


unsigned Encoder::estimateFrameSize(unsigned i_predictionOrder, unsigned i_frameSize, u_int32_t *freqs,
                                    unsigned i_nbSymbols, unsigned i_log2TotalFreq)
{
    unsigned codesSize = ceilf(computeShanonEntropy(freqs, i_nbSymbols, i_log2TotalFreq) * i_frameSize / 8.f);
    unsigned predictorSize = ceilf(i_predictionOrder * NB_QUANT_BITS / 8.f);
    unsigned headerSize = 8;

    return codesSize + predictorSize + headerSize;
}


float Encoder::computeShanonEntropy(u_int32_t *freqs, unsigned i_nbSymbols,
                                    unsigned i_log2TotalFreq)
{
    unsigned i_totalFreq = 1 << i_log2TotalFreq;
    float entropy = 0;

    for (unsigned i = 0; i < i_nbSymbols; ++i)
    {
        float p = (float)freqs[i] / i_totalFreq;
        entropy -= p * std::log2(p);
    }

    return entropy;
}
