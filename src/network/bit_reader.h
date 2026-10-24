#ifndef BIT_READER_H
#define BIT_READER_H

class BitReader {
public:
    unsigned int* data;
    int index;
    int bit;

    int ReadBits(int bits);
    int ReadSignedBits(int bits);

    int ReadBit()
    {
        int r = (data[index] & (1 << bit)) != 0;
        if (++bit == 32) {
            bit = 0;
            index++;
        }
        return r;
    }
};

#endif
