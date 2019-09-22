
#include <iostream>
#include <assert.h>
#include <math.h>

#include "parcorgenerator.h"


void ParCorGenerator::calculateLPCcoeffs(std::vector<int16_t> &data, unsigned i_dataOffset, unsigned i_dataSize,
                                         std::vector<float> &coeffs, std::vector<unsigned> &parCor,
                                         unsigned i_quant, Apodization &apodization)
{
    std::vector<float> window(i_dataSize);
    apodization.generateWindow(window);

    std::vector<float> windownedData(i_dataSize);
    for(unsigned i = 0; i < i_dataSize; i++)
        windownedData[i] = data[i + i_dataOffset] * window[i];

    // GET SIZE FROM INPUT VECTORS
    size_t N = i_dataSize - 1;
    size_t m = coeffs.size();

    // INITIALIZE R WITH AUTOCORRELATION COEFFICIENTS
    std::vector<float> R( m + 1, 0.0 );
    for ( size_t i = 0; i <= m; i++ )
    {
        for ( size_t j = 0; j <= N - i; j++ )
            R[ i ] += windownedData[ j ] * windownedData[ j + i ];
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

        parCor[k] = quantizeParCor(lambda, k + 1, i_quant);

        assert(parCor[k] < (1 << i_quant));

        lambda = getParCor(parCor[k], k + 1, i_quant);

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
}


void ParCorGenerator::getLPCcoeffsFromParCor(std::vector<unsigned> &parCor, std::vector<float> &coeffs, unsigned i_quant)
{
    // GET SIZE FROM INPUT VECTORS
    size_t m = parCor.size();

    // INITIALIZE Ak
    std::vector<float> Ak(m + 1, 0.0);
    Ak[0] = 1.0;

    // LEVINSON-DURBIN RECURSION
    for (size_t k = 0; k < m; k++)
    {
        // COMPUTE LAMBDA (PARCOR)
        float lambda = getParCor(parCor[k], k + 1, i_quant);

        // UPDATE Ak
        for (size_t n = 0; n <= (k + 1) / 2; n++)
        {
            double temp = Ak[k + 1 - n] + lambda * Ak[n];
            Ak[n] = Ak[n] + lambda * Ak[k + 1 - n];
            Ak[k + 1 - n] = temp;
        }
    }

    // ASSIGN COEFFICIENTS
    coeffs.assign(++Ak.begin(), Ak.end());
}


#define SQRT_2 1.414213562f


unsigned ParCorGenerator::quantizeParCor(float parcor, unsigned i_order, unsigned i_quant)
{
#if 0
    unsigned i_ret = std::roundf((parcor - (-QUANT_HALF_INTERVAL)) * (1 << i_quant) / (2 * QUANT_HALF_INTERVAL));
#else
    unsigned i_ret;
    switch (i_order)
    {
    case 1:
        i_ret = (1 << (i_quant - 1)) * SQRT_2 * sqrtf(parcor + 1.f);
        break;
    case 2:
        i_ret = (1 << (i_quant - 1)) * SQRT_2 * sqrtf(-parcor + 1.f);
        break;
    default:
        i_ret = (1 << (i_quant - 1)) * (parcor + 1.f);
        break;
    }
#endif
    i_ret = std::min(i_ret, (unsigned)(1 << i_quant) - 1);
    return i_ret;
}


float ParCorGenerator::getParCor(int quantizedParCor, unsigned i_order, unsigned i_quant)
{
#if 0
   float f_ret = quantizedParCor * (2 * QUANT_HALF_INTERVAL) / (1 << i_quant) - QUANT_HALF_INTERVAL;
#else
    float f_ret;
    switch (i_order)
    {
    case 1:
        f_ret = std::pow((float)quantizedParCor / (1 << (i_quant - 1)) / SQRT_2, 2.f) - 1.f;
        break;
    case 2:
        f_ret = 1.f - std::pow((float)quantizedParCor / (1 << (i_quant - 1)) / SQRT_2, 2.f);
        break;
    default:
        f_ret = (float)quantizedParCor / (1 << (i_quant - 1)) - 1.f;
        break;
    }
#endif
   return f_ret;
}
