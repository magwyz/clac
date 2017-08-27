#ifndef WAVREADER_H
#define WAVREADER_H

#include <string>
#include <vector>

#include <fstream>

#include <wavparameters.h>

class WavReader
{
public:
    WavReader();
    bool open(std::string path);
    bool readHeader(WavParameters &p);
    template<typename DataType> bool readAudioData(std::vector<DataType> &data)
    {
        char format[4];
        ifs.read(format, sizeof(format)); // format

        if (std::string(format, 4) != std::string("data"))
        {
            ifs.close();
            return false;
        }

        u_int32_t i_size;
        ifs.read((char*)&i_size, sizeof(i_size)); // size

        u_int32_t i_totalToRead = i_size / sizeof(DataType);

        for (unsigned i = 0; i < i_totalToRead; ++i)
        {
            DataType val;
            ifs.read((char*)&val, sizeof(DataType));
            if (!ifs.good())
            {
                ifs.close();
                return false;
            }

            data.push_back(val);
        }

        return true;
    }

private:
    std::ifstream ifs;
    WavParameters params;

    bool b_headerRead;
};


#endif // WAVREADER_H
