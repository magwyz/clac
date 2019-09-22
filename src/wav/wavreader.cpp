
#include <assert.h>

#include <wavreader.h>


WavReader::WavReader()
    : b_headerRead(false)
{ }


bool WavReader::open(std::string path)
{
    ifs.open(path, std::ios::binary);

    if (!ifs.good())
        return false;

    return true;
}


bool WavReader::readHeader(WavParameters& p)
{
    uint32_t data32;
    uint16_t data16;

    /* TODO: check each the file descriptor status after each read operation
     * with a macro. */

    // RIFF header
    ifs.read((char*)&data32, sizeof(data32)); // ChunkID
    ifs.read((char*)&data32, sizeof(data32)); // Chunks size
    char format[4];
    ifs.read(format, sizeof(format)); // format

    if (std::string(format, 4) != std::string("WAVE"))
    {
        ifs.close();
        return false;
    }

    // Fmt subchunk
    ifs.read(format, sizeof(format)); // Subchunk1ID
    if (std::string(format, 4) != std::string("fmt "))
    {
        ifs.close();
        return false;
    }

    ifs.read((char*)&data32, sizeof(data32)); // Subchunk1Size
    ifs.read((char*)&data16, sizeof(data16)); // AudioFormat

    ifs.read((char*)&data16, sizeof(data16)); // NumChannels
    ifs.read((char*)&data32, sizeof(data32)); // SampleRate

    ifs.read((char*)&data32, sizeof(data32)); // ByteRate

    ifs.read((char*)&data16, sizeof(data16)); // BlockAlign
    ifs.read((char*)&data16, sizeof(data16)); // BitsPerSample

    p = params;

    while (ifs.good())
    {
        ifs.read(format, sizeof(format)); // format

        if (std::string(format, 4) == std::string("data"))
            break;

        ifs.read((char*)&data32, sizeof(data32)); // Chunks size
        std::streampos pos = ifs.tellg();
        ifs.seekg(pos + (std::streampos)data32); // Skip the chunk
    }

    return true;
}
