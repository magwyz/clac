#include <iostream>
#include <fstream>

#include <map>
#include <vector>
#include <algorithm>

#include <main.hpp>
#include <wavreader.h>
#include <wavwriter.h>

#include <encoder.h>
#include <decoder.h>

int encode(std::string inPath, std::string outPath);
int decode(std::string inPath, std::string outPath);
void printUsage();



int main(int argc, char* argv[])
{
    bool b_ret;

    std::cout << "💩 CLAC: Crappy Lossless Audio Codec 💩" << std::endl;

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
        char p_frameData[MAX_FRAME_SIZE] = {0};
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

    unsigned i = 0;
    while (offset < fileSize)
    {
        std::cout << (float)offset / fileSize * 100.f << "%" << std::endl;
        int ret = dec.decodeFrame(p_buffer, offset, decodedFrameData);
        if (ret)
            abort();
    }

    WavWriter writer;
    bool b_ret = writer.open(outPath);
    if (!b_ret)
    {
        std::cout << "Error opening the output Wav file." << std::endl;
        return 1;
    }

    WavParameters p;
    p.i_bitsPerSamples = 16;
    p.i_nbChannels = 1;
    p.i_sampleRate = 44100;

    b_ret = writer.writeHeader(p, decodedFrameData.size() * 2);
    if (!b_ret)
    {
        std::cout << "Error writing the Wav header of the output file." << std::endl;
        return 1;
    }

    b_ret = writer.writeAudioData(decodedFrameData, p);
    if (!b_ret)
    {
        std::cout << "Error writing the Wav audio data of the output file." << std::endl;
        return 1;
    }

    return 0;
}


void printUsage()
{
    std::cout << "Usage: clac -e/-d SOURCE DEST" << std::endl;
}
