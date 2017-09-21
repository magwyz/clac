/*****************************************************************************
* Copyright (C) 2012 Adrien Maglo
*
* This file is part of RangeCoder.
*
* RangeCoder is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* RangeCoder is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with RangeCoder.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include <stdio.h>
#include "rangeCoder.h"


void initRangeCoder(RangeCoder *rc, unsigned char *p_data)
{
    // Reset the variables.
    rc->p_data = p_data;
    rc->i_dataOffset = 0;

    rc->i_message = 0;
    rc->i_prodFreq = 1;

    rc->i_nbBitsShift = 0;

    rc->c_bitMask = 128;
    rc->i_nbLeadingBitsSkipped = 0;
}


void startEncoding(RangeCoder *rc, unsigned char *p_data)
{
    initRangeCoder(rc, p_data);
}


unsigned stopEncoding(RangeCoder *rc)
{
    writeAndShiftBits(rc, MESSAGE_BIT_LENGTH);
    return rc->i_dataOffset + 1;
}


void encodeFrequency(RangeCoder *rc, unsigned i_log2TotFreq,
                     unsigned i_symbolFreq, uint64_t i_symbolCumFreq)
{
    writeAndShiftBits(rc, i_log2TotFreq - rc->i_nbBitsShift);

    rc->i_message <<= i_log2TotFreq - rc->i_nbBitsShift;
    rc->i_message += i_symbolCumFreq * rc->i_prodFreq;

    uint64_t i_mask = 0xffffffffffffffff >> (MESSAGE_BIT_LENGTH - i_log2TotFreq);
    rc->i_prodFreq = safeProduct(rc, rc->i_prodFreq, i_symbolFreq, i_mask);
}


void writeAndShiftBits(RangeCoder *rc, unsigned i_nbBits)
{
    unsigned char c_shift = MESSAGE_BIT_LENGTH - 1;

    for (unsigned i = 0; i < i_nbBits; ++i)
    {
        // Set to 1 the current bit if needed.
        if (((rc->i_message >> c_shift) & 1) > 0)
            rc->p_data[rc->i_dataOffset] |= rc->c_bitMask;

        c_shift--;

        // This test is to not write the leading zeros.
        if (rc->i_nbLeadingBitsSkipped == MESSAGE_BIT_LENGTH)
        {
            rc->c_bitMask >>= 1;
            if (rc->c_bitMask == 0)
            {
                rc->i_dataOffset++;
                rc->c_bitMask = 128;
            }
        }
        else
            rc->i_nbLeadingBitsSkipped++;
    }
}


uint64_t safeProduct(RangeCoder *rc, uint64_t x, uint64_t y, uint64_t i_mask)
{
    uint64_t i_res = x * y;

    // Shift right the message var as much as possible.
    rc->i_nbBitsShift = 0;
    while (i_res > i_mask)
    {
        i_res >>= 1;
        rc->i_nbBitsShift++;
    }

    return i_res;
}


void startDecoding(RangeCoder *rc, unsigned char *p_data)
{
    initRangeCoder(rc, p_data);
}


unsigned stopDecoding(RangeCoder *rc)
{
    return rc->i_dataOffset + 1;
}


void readAndShiftBits(RangeCoder *rc, unsigned i_nbBits)
{
    for (unsigned i = 0; i < i_nbBits; ++i)
    {
        // Write one bit.
        rc->i_message <<= 1;
        if ((rc->p_data[rc->i_dataOffset] & rc->c_bitMask) == rc->c_bitMask)
            rc->i_message |= 1;
        rc->c_bitMask >>= 1;

        // Test if we need to begin to write a new byte.
        if (rc->c_bitMask == 0)
        {
            rc->i_dataOffset++;
            rc->c_bitMask = 128;
        }
    }
}


uint64_t decodeCumFreq(RangeCoder *rc, unsigned i_log2TotFreq)
{
    readAndShiftBits(rc, i_log2TotFreq - rc->i_nbBitsShift);
    return rc->i_message / rc->i_prodFreq;
}


void updateAfterDecoding(RangeCoder *rc, unsigned i_log2TotFreq,
                         unsigned i_symbolFreq, uint64_t i_symbolCumFreq)
{
    rc->i_message -= i_symbolCumFreq * rc->i_prodFreq;

    uint64_t i_mask = 0xffffffffffffffff >> (MESSAGE_BIT_LENGTH - i_log2TotFreq);
    rc->i_prodFreq = safeProduct(rc, rc->i_prodFreq, i_symbolFreq, i_mask);
}


void writeBits(RangeCoder *rc, unsigned i_nbBits, uint64_t data)
{
    encodeFrequency(rc, i_nbBits, 1, data);
}


uint64_t readBits(RangeCoder *rc, unsigned i_nbBits)
{
    uint64_t ret = decodeCumFreq(rc, i_nbBits);
    updateAfterDecoding(rc, i_nbBits, 1, ret);
    return ret;
}
