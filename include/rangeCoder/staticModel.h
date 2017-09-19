#ifndef STATICMODEL_H
#define STATICMODEL_H

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

#endif
