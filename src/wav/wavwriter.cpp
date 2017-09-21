
#include <wavwriter.h>


bool WavWriter::open(std::string path)
{
    ofs.open(path, std::ios::binary);

    if (!ofs.good())
        return false;

    return true;
}


bool WavWriter::writeHeader(WavParameters& p, size_t i_audioDataSize)
{
    uint32_t data32;
    uint16_t data16;

    // RIFF header
    ofs.write("RIFF", 4);
    data32 = 36 + i_audioDataSize;
    ofs.write((char*)&data32, sizeof(data32)); // Chunks size
    char format[4];
    ofs.write("WAVE", 4); // format

    // Fmt subchunk
    ofs.write("fmt ", 4); // Subchunk1ID

    data32 = 16;
    ofs.write((char*)&data32, sizeof(data32)); // Subchunk1Size

    data16 = 1;
    ofs.write((char*)&data16, sizeof(data16)); // AudioFormat

    data16 = p.i_nbChannels;
    ofs.write((char*)&data16, sizeof(data16)); // NumChannels

    data32 = p.i_sampleRate;
    ofs.write((char*)&data32, sizeof(data32)); // SampleRate

    data32 = p.i_sampleRate * p.i_nbChannels * p.i_bitsPerSamples / 8;
    ofs.write((char*)&data32, sizeof(data32)); // ByteRate

    data16 = p.i_nbChannels * p.i_bitsPerSamples / 8;
    ofs.write((char*)&data16, sizeof(data16)); // BlockAlign

    data16 = p.i_bitsPerSamples;
    ofs.write((char*)&data16, sizeof(data16)); // BitsPerSample

    return true;
}
