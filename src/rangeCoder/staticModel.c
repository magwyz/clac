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

#include "staticModel.h"


StaticModel *createStaticModel(unsigned i_nbSymbols, unsigned i_log2TotFreq,
                               uint32_t *freqs)
{
    if (i_nbSymbols < 2)
        goto error;

    StaticModel *sm = malloc(sizeof(StaticModel));
    if (sm == NULL)
        goto error;

    sm->i_nbSymbols = i_nbSymbols;
    sm->freqs = malloc(i_nbSymbols * sizeof(uint32_t));
    sm->cumFreqs = malloc((i_nbSymbols + 1) * sizeof(uint32_t));

    if (sm->freqs == NULL
        || sm->cumFreqs == NULL)
        goto error;

    sm->cumFreqs[i_nbSymbols] = 1 << i_log2TotFreq;

    uint32_t i_cumFreq = 0;
    for (unsigned i = 0; i < i_nbSymbols; ++i)
    {
        sm->freqs[i] = freqs[i];
        sm->cumFreqs[i] = i_cumFreq;
        i_cumFreq += freqs[i];
    }

    return sm;

error:
    return NULL;
}


void deleteStaticModel(StaticModel *sm)
{
    free(sm->freqs);
    free(sm->cumFreqs);

    free(sm);
}


unsigned getSymbolFromCumFreqSM(StaticModel *sm, uint32_t i_cumFreq)
{
    unsigned i_sym = sm->i_nbSymbols - 1;

    while (i_cumFreq < sm->cumFreqs[i_sym])
        --i_sym;

    return i_sym;
}


void getSymbolFrequenciesSM(StaticModel *sm, unsigned i_sym,
                            unsigned *i_freq, unsigned *i_cumFreq)
{
    *i_freq = sm->cumFreqs[i_sym + 1] - sm->cumFreqs[i_sym];
    *i_cumFreq = sm->cumFreqs[i_sym];
}
