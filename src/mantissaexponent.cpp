
#include <assert.h>

#include <mantissaexponent.h>


unsigned mantissaExponentToNumber(unsigned mantissa, unsigned exponent)
{
    return mantissa * (1 << exponent);
}


unsigned numberToMantissaExponent(unsigned inNumber,
                                  unsigned nbBitsMantissa, unsigned nbBitsExponent,
                                  unsigned &mantissa, unsigned &exponent)
{
    mantissa = inNumber;
    exponent = 0;

    while (mantissa >= (1 << nbBitsMantissa))
    {
        mantissa >>= 1;
        exponent++;
        assert(exponent < (1 << nbBitsExponent));
    }

    return mantissaExponentToNumber(mantissa, exponent);
}
