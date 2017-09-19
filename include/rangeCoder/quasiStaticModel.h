#ifndef QUASISTATICMODEL_H
#define QUASISTATICMODEL_H

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>


typedef struct
{
    unsigned i_nbSymbols; // Number of symbols.
    unsigned i_totalFreq; // Total frequeny of the model.

    unsigned i_targetRescale; // The rescale interval set by the user.
    unsigned i_rescale; // The current rescale interval.

    unsigned i_rescaleIncr; // Rescale increment.

    unsigned i_nbSymbolsBeforeRescale; // Number of symbols before next rescale.
    unsigned i_nbPlusOneSymbolsBeforeRescale; // Number of symbols that must get a +1 increment.

    uint32_t *newFreqs; // Symbol frequencies.
    uint32_t *cumFreqs; // Cumulative frequencies.
} QuasiStaticModel;


/**
  * Create a new quasi static model.
  * @param i_nbSymbols the number of symbols of the model.
  * @param i_log2TotFreq the base 2 log of the total frequency.
  * @param i_targetRescale the final rescale interval.
  * @param freqs the intial frequencies of each symbol of the model.
  * If null then a equal frequency is given to each symbol.
  * @return a pointer on the new static model structure.
  */
QuasiStaticModel *createQuasiStaticModel(unsigned i_nbSymbols, unsigned i_log2TotFreq,
                                         unsigned i_targetRescale, uint32_t *freqs);


/**
  * Delete a quasi static model.
  * @param sm a pointer on the static model to delete.
  **/
void deleteQuasiStaticModel(QuasiStaticModel *qsm);


/**
  * Reset the frequencies of a quasi static model.
  * @param qsm a pointeur on the quasi static model.
  * @param freqs the frequencies to initialize the model.
  */
void resetFrequenciesQSM(QuasiStaticModel *qsm, uint32_t *freqs);


/**
  * Rescale a quasi static model.
  * @param qsm a pointeur on the quasi static model.
  */
void rescaleQSM(QuasiStaticModel *qsm);


/**
  * Update a quasi static model with a new symbol.
  * @param qsm a pointeur on the quasi static model.
  * @param i_sym the symbol to add.
  */
void updateQSM(QuasiStaticModel *qsm, unsigned i_sym);


/**
  * Get the frequencies of a symbol from the quasi static model.
  * @param qsm a pointeur on the quasi static model.
  * @param i_sym the symbol.
  * @param i_freq a pointer to return the frequency of the symbol.
  * @param i_cumFreq a pointer to return the cumulative frequency of the symbol.
  **/
void getSymbolFrequenciesQSM(QuasiStaticModel *qsm, unsigned i_sym,
                             unsigned *i_freq, unsigned *i_cumFreq);


/**
  * Get a symbol from a cumulative frequency.
  * @param qsm a pointeur on the quasi static model.
  * @param i_cumFreq the cumulative frequency of the symbol.
  * @return the symbol.
  */
unsigned getSymbolFromCumFreqQSM(QuasiStaticModel *qsm, uint32_t i_cumFreq);

#endif
