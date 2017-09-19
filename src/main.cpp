#include <iostream>
#include <fstream>

#include <map>
#include <vector>
#include <algorithm>

#include <main.hpp>
#include <wavreader.h>

#include <encoder.h>
#include <decoder.h>

int encode(std::string inPath, std::string outPath);
int decode(std::string inPath, std::string outPath);
void printUsage();



int main(int argc, char* argv[])
{
    bool b_ret;

    if (argc != 4)
    {
        printUsage();
        return 1;
    }

    std::string mode(argv[1]);
    std::string inPath(argv[2]);
    std::string outPath(argv[3]);

    if (mode == std::string("-e"))
    {
        return encode(inPath, outPath);
    }
    else if (mode == std::string("-d"))
    {
        return decode(inPath, outPath);
    }
    else
    {
        printUsage();
        return 1;
    }


#if 0
    rc = (RangeCoder*)malloc(sizeof(RangeCoder));
    qsm = createQuasiStaticModel(i_nbSymbols, i_log2TotalFreq,
                                1 << (i_log2TotalFreq - 1), NULL);

    startDecoding(rc, p_data);
    printf("Decoded data:\n");

    for (unsigned i = 0; i < codes.size(); ++i)
    {
        uint32_t i_cumFreqDec = decodeCumFreq(rc, i_log2TotalFreq);

        unsigned i_sym = getSymbolFromCumFreqQSM(qsm, i_cumFreqDec);
        int16_t c = i_sym + i_codeMin;

        if (c != codes[i])
        {
            std::cout << i << ": " << c << " " << codes[i] << std::endl;
            abort();
        }

        unsigned i_freq, i_cumFreq;
        getSymbolFrequenciesQSM(qsm, i_sym, &i_freq, &i_cumFreq);
        updateAfterDecoding(rc, i_log2TotalFreq, i_freq, i_cumFreq);
        updateQSM(qsm, i_sym);
    }

    unsigned i_nbSamples = stopDecoding(rc);
    printf("Data size: %u\n", i_nbSamples);

    deleteQuasiStaticModel(qsm);
    free(rc);
#endif

    return 0;
}


int encode(std::string inPath, std::string outPath)
{
    WavReader reader;
    bool b_ret = reader.open(inPath);
    if (!b_ret)
    {
        std::cout << "Error opening the input Wav file." << std::endl;
        return 1;
    }

    WavParameters p;
    b_ret = reader.readHeader(p);
    if (!b_ret)
    {
        std::cout << "Error reading the Wav header of the input file." << std::endl;
        return 1;
    }

    std::vector<int16_t> ad;
    b_ret = reader.readAudioData(ad);
    if (!b_ret)
    {
        std::cout << "Error reading the Wav audio data of the input file." << std::endl;
        return 1;
    }

    Encoder enc;
    size_t offset = 0;
    char* p_buffer = (char*)malloc(100 * 1024 * 1024);

    unsigned i_nbFrames = 0;
    unsigned i_frameStart = 0;
    unsigned i_lastRefFrame = 0;

    std::cout << ad.size() << std::endl;

    while (i_frameStart < ad.size())
    {
        char p_frameData[MAX_FRAME_SIZE];
        unsigned i_frameCompressedSize;
        unsigned i_frameSizeSelected;
        bool b_refFrame = i_frameStart - i_lastRefFrame >= 48000
                          || i_frameStart == 0 ? true : false;

        enc.encodeFrame(ad, i_frameStart, i_frameSizeSelected, p_frameData, i_frameCompressedSize, b_refFrame);
        std::copy(p_frameData, p_frameData + i_frameCompressedSize, p_buffer + offset);

        offset += i_frameCompressedSize;
        i_nbFrames++;

        std::cout << i_frameStart * 100.f / ad.size() << "%" << std::endl;

        if (b_refFrame)
            i_lastRefFrame = i_frameStart;

        i_frameStart += i_frameSizeSelected;
    }

    std::cout << "Total size: " << offset << std::endl;
    std::cout << "Nb frames: " << i_nbFrames << std::endl;

    std::ofstream outFile;
    outFile.open(outPath, std::ios::out | std::ios::binary);
    outFile.write(p_buffer, offset);
    outFile.close();

    return 0;
}


int decode(std::string inPath, std::string outPath)
{
    char* p_buffer = (char*)malloc(100 * 1024 * 1024);

    std::ifstream inFile;
    inFile.open(inPath, std::ios::in | std::ios::binary);

    inFile.seekg(0, std::ios::end);
    size_t fileSize = inFile.tellg();
    inFile.seekg(0, std::ios::beg);

    inFile.read(p_buffer, fileSize);

    Decoder dec;
    size_t offset = 0;

    std::vector<int16_t> decodedFrameData;

    dec.decodeFrame(p_buffer, offset, decodedFrameData);

    return 0;
}


void printUsage()
{
    std::cout << "Usage: clac -e/-d SOURCE DEST" << std::endl;
}
