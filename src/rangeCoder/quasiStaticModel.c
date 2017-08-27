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

#include "quasiStaticModel.h"


QuasiStaticModel *createQuasiStaticModel(unsigned i_nbSymbols, unsigned i_log2TotFreq,
                                         unsigned i_targetRescale, uint32_t *freqs)
{
    if (i_nbSymbols < 2)
        goto error;

    QuasiStaticModel *qsm = malloc(sizeof(QuasiStaticModel));
    if (qsm == NULL)
        goto error;

    qsm->i_totalFreq = 1 << i_log2TotFreq;
    qsm->i_nbSymbols = i_nbSymbols;
    qsm->i_targetRescale = i_targetRescale;
    qsm->newFreqs = malloc(i_nbSymbols * sizeof(uint32_t));
    qsm->cumFreqs = malloc((i_nbSymbols + 1) * sizeof(uint32_t));
    qsm->cumFreqs[i_nbSymbols] = qsm->i_totalFreq;

    if (qsm->newFreqs == NULL
        || qsm->cumFreqs == NULL)
        goto error;

    qsm->cumFreqs[i_nbSymbols] = qsm->i_totalFreq;

    resetFrequenciesQSM(qsm, freqs);

    return qsm;

error:
    return NULL;
}


void deleteQuasiStaticModel(QuasiStaticModel *qsm)
{
    free(qsm->newFreqs);
    free(qsm->cumFreqs);

    free(qsm);
}


void resetFrequenciesQSM(QuasiStaticModel *qsm, uint32_t *freqs)
{
    qsm->i_rescale = 2;

    if (freqs == NULL)
    {
        // Set an equal frequency to each symbol.
        // Add one to the first frequencies to fill the entire interval.
        unsigned i_initFreq = qsm->i_totalFreq / qsm->i_nbSymbols;
        unsigned i_endPlusOneFreq = qsm->i_totalFreq % qsm->i_nbSymbols;

        unsigned i;
        for (i = 0; i < i_endPlusOneFreq; ++i)
            qsm->newFreqs[i] = i_initFreq + 1;
        for (; i < qsm->i_nbSymbols; ++i)
            qsm->newFreqs[i] = i_initFreq;
    }
    else
    {
        // Initialize the model with the provided frequencies.
        for (unsigned i = 0; i < qsm->i_nbSymbols; ++i)
            qsm->newFreqs[i] = freqs[i];
    }

    rescaleQSM(qsm);
}


void rescaleQSM(QuasiStaticModel *qsm)
{
    // Compute the current cumulative frequencies
    // Initialize the next symbol frequencies.
    uint32_t i_missingFreq = qsm->i_totalFreq;
    uint32_t i_cumFreq = qsm->i_totalFreq;

    for (int i = qsm->i_nbSymbols - 1; i >= 0; --i)
    {
        i_cumFreq -= qsm->newFreqs[i];
        qsm->cumFreqs[i] = i_cumFreq;

        qsm->newFreqs[i] >>= 1;
        if (qsm->newFreqs[i] < 1)
          qsm->newFreqs[i] = 1;

        i_missingFreq -= qsm->newFreqs[i];
    }

    // Sanity check.
    assert(qsm->cumFreqs[0] == 0);

    // Determine the next rescale interval.
    if (qsm->i_rescale < qsm->i_targetRescale)
    {
        // Double the rescale interval.
        qsm->i_rescale <<= 1;
        if (qsm->i_rescale > qsm->i_targetRescale)
            qsm->i_rescale = qsm->i_targetRescale;
    }

    qsm->i_rescaleIncr = i_missingFreq / qsm->i_rescale;
    qsm->i_nbPlusOneSymbolsBeforeRescale = i_missingFreq % qsm->i_rescale;
    qsm->i_nbSymbolsBeforeRescale = qsm->i_rescale - qsm->i_nbPlusOneSymbolsBeforeRescale;
}


void updateQSM(QuasiStaticModel *qsm, unsigned i_sym)
{
    if (qsm->i_nbPlusOneSymbolsBeforeRescale == 0
        && qsm->i_nbSymbolsBeforeRescale == 0)
        rescaleQSM(qsm);

    if (qsm->i_nbPlusOneSymbolsBeforeRescale > 0)
    {
        qsm->i_nbPlusOneSymbolsBeforeRescale--;
        qsm->newFreqs[i_sym] += qsm->i_rescaleIncr + 1;
    }
    else
    {
        assert(qsm->i_nbSymbolsBeforeRescale > 0);
        qsm->i_nbSymbolsBeforeRescale--;
        qsm->newFreqs[i_sym] += qsm->i_rescaleIncr;
    }
}


void getSymbolFrequenciesQSM(QuasiStaticModel *qsm, unsigned i_sym,
                             unsigned *i_freq, unsigned *i_cumFreq)
{
    *i_freq = qsm->cumFreqs[i_sym + 1] - qsm->cumFreqs[i_sym];
    *i_cumFreq = qsm->cumFreqs[i_sym];
}


unsigned getSymbolFromCumFreqQSM(QuasiStaticModel *qsm, uint32_t i_cumFreq)
{
    unsigned i_symHigh = qsm->i_nbSymbols;
    unsigned i_symLow = 0;

    while (i_symHigh - i_symLow > 1)
    {
        unsigned i_symMid = (i_symHigh + i_symLow) / 2;
        if (i_cumFreq < qsm->cumFreqs[i_symMid])
            i_symHigh = i_symMid;
        else
            i_symLow = i_symMid;
    }

    return i_symLow;
}
