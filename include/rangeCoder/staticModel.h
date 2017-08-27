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

#include <stdlib.h>
#include <stdint.h>


typedef struct
{
    unsigned i_nbSymbols;

    uint32_t *freqs; // Symbol frequencies.
    uint32_t *cumFreqs; // Cumulative frequencies.
} StaticModel;


/**
  * Create a new static model.
  * @param i_nbSymbols the number of symbols of the model.
  * @param i_log2TotFreq the base 2 log of the total frequency.
  * @param freqs the frequencies of each symbol of the model.
  * @return a pointer on the new static model structure.
  */
StaticModel *createStaticModel(unsigned i_nbSymbols, unsigned i_log2TotFreq,
                               uint32_t *freqs);


/**
  * Delete a static model.
  * @param sm a pointer on the static model to delete.
  **/
void deleteStaticModel(StaticModel *sm);


/**
  * Get a symbol from a cumulative frequency.
  * @param sm a pointer on the static model to delete.
  * @param i_cumFreq the cumulative frequency of the symbol.
  * @return the symbol.
  */
unsigned getSymbolFromCumFreqSM(StaticModel *sm, uint32_t i_cumFreq);


/**
  * Get the frequencies of a symbol from the static model.
  * @param qsm a pointeur on the static model.
  * @param i_sym the symbol.
  * @param i_freq a pointer to return the frequency of the symbol.
  * @param i_cumFreq a pointer to return the cumulative frequency of the symbol.
  **/
void getSymbolFrequenciesSM(StaticModel *sm, unsigned i_sym,
                            unsigned *i_freq, unsigned *i_cumFreq);
