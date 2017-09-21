#ifndef WAVWRITER_H
#define WAVWRITER_H

#include <string>
#include <vector>

#include <fstream>

#include <wavparameters.h>

class WavWriter
{
public:
    bool open(std::string path);
    bool writeHeader(WavParameters &p, size_t i_audioDataSize);
    template<typename DataType> bool writeAudioData(std::vector<DataType> &data,
                                                    WavParameters &p)
    {
        uint32_t data32;

        ofs.write("data", 4); // format

        data32 = data.size() * p.i_bitsPerSamples / 8;
        ofs.write((char*)&data32, sizeof(data32)); // size

        for (unsigned i = 0; i < data.size(); ++i)
        {
            ofs.write((char*)&data[i], sizeof(DataType));
            if (!ofs.good())
            {
                ofs.close();
                return false;
            }
        }

        return true;
    }

private:
    std::ofstream ofs;
    WavParameters params;
};

#endif // WAVWRITER_H
