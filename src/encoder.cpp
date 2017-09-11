
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <future>
#include <cstring>

#include <encoder.h>


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
    int i_codeMin = *std::min_element(codes.begin(), codes.end());
    int i_codeMax = *std::max_element(codes.begin(), codes.end());

    if (i_codeMin == i_codeMax) // We must have at least two symbols for the range coder model.
        i_codeMax = i_codeMin + 1;

    const unsigned i_nbSymbols = i_codeMax - i_codeMin + 1;
    const unsigned i_log2TotalFreq = 20;

    int16_t b = calculateBParameter(codes);

    uint32_t freqs[i_nbSymbols];
    buildRangeCoderDistribution(i_codeMin, i_codeMax, freqs, b, i_log2TotalFreq);



#if 0
    // Distribution vizualization
    std::ofstream ofsFreq;

    std::stringstream ssFreq;
    static unsigned dis_id = 0;
    ssFreq << "distribution_" << dis_id++ << ".dat";

    ofsFreq.open(ssFreq.str());
    assert(ofsFreq.good());

    for (int i = 0; i < i_nbSymbols; ++i)
        ofsFreq << freqs[i]  << "," << hist[i + i_codeMin] << std::endl;

    ofsFreq.close();
#endif

    RangeCoder* rc = (RangeCoder*)malloc(sizeof(RangeCoder));
    QuasiStaticModel* qsm = createQuasiStaticModel(i_nbSymbols, i_log2TotalFreq,
                                 1 << (i_log2TotalFreq - 1), freqs);

    startEncoding(rc, (unsigned char*)p_buffer);

    // Write the frame header.
    writeBits(rc, 16, 0b1111111111111110);

    // Write the b parameter.
    writeBits(rc, 15, b);

    // Write the code min and max.
    writeBits(rc, 15, i_codeMin + (1 << 15));
    writeBits(rc, 15, i_codeMax + (1 << 15));

    // Write the predictor order.
    writeBits(rc, 4, parCor.size() - 1);

    // Write the predictor PARCOR.
    for (unsigned i = 0; i < parCor.size(); ++i)
        writeBits(rc, NB_QUANT_BITS, parCor[i]);

    // Write the value of the first sample.
    if (b_refFrame)
        writeBits(rc, 16, data[i_dataOffset] + (1 << 15));

    for (unsigned i = 0; i < codes.size(); ++i)
    {
        unsigned i_freq, i_cumFreq;

        const int32_t symb = codes[i] - i_codeMin;
        assert(symb >= 0);
        assert(symb < (int32_t)i_nbSymbols);
        getSymbolFrequenciesQSM(qsm, symb, &i_freq, &i_cumFreq);
        encodeFrequency(rc, i_log2TotalFreq, i_freq, i_cumFreq);
        //updateQSM(qsm, symb);
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


inline float exp1(float x) {
  x = 1.f + x / 256.f;
  x *= x; x *= x; x *= x; x *= x;
  x *= x; x *= x; x *= x; x *= x;
  return x;
}


void Encoder::buildRangeCoderDistribution(int i_codeMin, int i_codeMax, uint32_t freqs[], uint16_t b,
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
    for (unsigned i = 0; i < i_nbSymbols; ++i)
    {
        freqs[i] = 1 + floorf(pf_freqs[i] * alpha);
        sum2 += freqs[i];
    }

    unsigned i_missingCount = i_totalFreq - sum2;
    for (unsigned i = 0; i < i_missingCount; ++i)
        freqs[i]++;
}


void Encoder::selectBestParameters(std::vector<int16_t> &data, unsigned i_dataOffset, unsigned &i_selectedFrameSize,
                                   std::vector<int16_t> &predictions, std::vector<unsigned> &ParCor,
                                   bool b_refFrame)
{
    unsigned frameSizes[] = {128, 256, 384, 512, 640, 768, 896, 1024, 1152, 1280, 1408, 1536, 1664, 1792, 1920, 2048};
    //unsigned frameSizes[] = {128, 256, 384, 512, 640, 768, 896, 1024};
    //unsigned frameSizes[] = {512, 1024, 1536, 2048, 2560, 3072, 3584, 4096};

    float i_lastSizePerSample = 0;
    unsigned i_lastOrder = 0;

    const unsigned i_nbFrameSizes = sizeof(frameSizes) / sizeof(*frameSizes);

    for (unsigned i = 0; i < i_nbFrameSizes; ++i)
    {
        unsigned i_frameSize = std::min((unsigned)data.size() - i_dataOffset, frameSizes[i]);

        for (unsigned order = 1; order <= 16; ++order)
        {
            std::vector<int16_t> newPredictions;

            std::vector<float> newLPCCoeff(order);
            std::vector<unsigned> newParCor(order);

            if (!b_refFrame)
                calculateLPCcoeffs(data, i_dataOffset - order, i_frameSize + order, newLPCCoeff, newParCor);
            else
                calculateLPCcoeffs(data, i_dataOffset, i_frameSize, newLPCCoeff, newParCor);

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

            // Find the min and max.
            int i_codeMin = *std::min_element(newPredictions.begin(), newPredictions.end());
            int i_codeMax = *std::max_element(newPredictions.begin(), newPredictions.end());

            if (i_codeMin == i_codeMax) // We must have at least two symbols for the range coder model.
                i_codeMax = i_codeMin + 1;

            const unsigned i_nbSymbols = i_codeMax - i_codeMin + 1;
            const unsigned i_log2TotalFreq = 20;

            //int16_t b = calculateBParameter(newPredictions);

            //uint32_t freqs[i_nbSymbols];
            //buildRangeCoderDistribution(i_codeMin, i_codeMax, freqs, b, i_log2TotalFreq);


            //float i_sizePerSample = (float)estimateFrameSize(order, i_frameSize, freqs, i_nbSymbols, i_log2TotalFreq) / i_frameSize;
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


void Encoder::calculateLPCcoeffs(std::vector<int16_t> &data, unsigned i_dataOffset, unsigned i_dataSize,
                                 std::vector<float> &coeffs, std::vector<unsigned> &parCor)
{
    // GET SIZE FROM INPUT VECTORS
    size_t N = i_dataSize - 1;
    size_t m = coeffs.size();

    // INITIALIZE R WITH AUTOCORRELATION COEFFICIENTS
    std::vector<float> R( m + 1, 0.0 );
    for ( size_t i = 0; i <= m; i++ )
    {
        for ( size_t j = 0; j <= N - i; j++ )
            R[ i ] += data[ i_dataOffset + j ] * data[ i_dataOffset + j + i];
    }

#if 0
    std::cout << "  R" << std::endl;
    for (unsigned i = 0; i < R.size(); ++i)
        std::cout << i << ": " << R[i] << std::endl;
#endif

    // INITIALIZE Ak
    std::vector<float> Ak( m + 1, 0.0 );
    Ak[ 0 ] = 1.0;
    // INITIALIZE Ek
    double Ek = R[0];

    // LEVINSON-DURBIN RECURSION
    for ( size_t k = 0; k < m; k++ )
    {
        // COMPUTE LAMBDA (PARCOR)
        float lambda = 0.0;
        for ( size_t j = 0; j <= k; j++ )
            lambda -= Ak[ j ] * R[ k + 1 - j ];

        if (lambda != 0.f)
            lambda /= Ek;

        if (lambda > 1.f) lambda = 1.f;
        if (lambda < -1.f) lambda = -1.f;

        parCor[k] = quantizeParCor(lambda, k + 1);

        assert(parCor[k] < (1 << NB_QUANT_BITS));

        lambda = getParCor(parCor[k], k + 1);

        // UPDATE Ak
        for ( size_t n = 0; n <= ( k + 1 ) / 2; n++ )
        {
            double temp = Ak[ k + 1 - n ] + lambda * Ak[ n ];
            Ak[ n ] = Ak[ n ] + lambda * Ak[ k + 1 - n ];
            Ak[ k + 1 - n ] = temp;
        }
        // UPDATE Ek
        Ek *= 1.0 - lambda * lambda;
    }

    // ASSIGN COEFFICIENTS
    coeffs.assign(++Ak.begin(), Ak.end());

    /*std::cout << "--------" << std::endl;
    for (unsigned i = 0; i<coeffs.size(); ++i)
        std::cout << coeffs[i] << std::endl;*/
}


#define SQRT_2 1.414213562f


unsigned Encoder::quantizeParCor(float parcor, unsigned i_order)
{
#if 0
    unsigned i_ret = std::roundf((parcor - (-QUANT_HALF_INTERVAL)) * (1 << NB_QUANT_BITS) / (2 * QUANT_HALF_INTERVAL));
#else
    unsigned i_ret;
    switch (i_order)
    {
    case 1:
        i_ret = (1 << (NB_QUANT_BITS - 1)) * SQRT_2 * sqrtf(parcor + 1.f);
        break;
    case 2:
        i_ret = (1 << (NB_QUANT_BITS - 1)) * SQRT_2 * sqrtf(-parcor + 1.f);
        break;
    default:
        i_ret = (1 << (NB_QUANT_BITS - 1)) * (parcor + 1.f);
        break;
    }
#endif
    i_ret = std::min(i_ret, (unsigned)(1 << NB_QUANT_BITS) - 1);
    return i_ret;
}


float Encoder::getParCor(int quantizedParCor, unsigned i_order)
{
#if 0
   float f_ret = quantizedParCor * (2 * QUANT_HALF_INTERVAL) / (1 << NB_QUANT_BITS) - QUANT_HALF_INTERVAL;
#else
    float f_ret;
    switch (i_order)
    {
    case 1:
        f_ret = std::pow((float)quantizedParCor / (1 << (NB_QUANT_BITS - 1)) / SQRT_2, 2.f) - 1.f;
        break;
    case 2:
        f_ret = 1.f - std::pow((float)quantizedParCor / (1 << (NB_QUANT_BITS - 1)) / SQRT_2, 2.f);
        break;
    default:
        f_ret = (float)quantizedParCor / (1 << (NB_QUANT_BITS - 1)) - 1.f;
        break;
    }
#endif
   return f_ret;
}
