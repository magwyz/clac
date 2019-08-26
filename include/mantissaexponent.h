#ifndef MANTISSAEXPONENT_H
#define MANTISSAEXPONENT_H


unsigned mantissaExponentToNumber(unsigned mantissa, unsigned exponent);
unsigned numberToMantissaExponent(unsigned inNumber,
                                  unsigned nbBitsMantissa, unsigned nbBitsExponent,
                                  unsigned &mantissa, unsigned &exponent);


#endif // MANTISSAEXPONENT_H
