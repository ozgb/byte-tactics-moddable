#ifndef BIT_WRITER_H
#define BIT_WRITER_H

class BitWriter {
public:
    int index;
    int bit;
    int capacity;
    unsigned int* data;
    unsigned int buffer[0x100];

    BitWriter();
    void FreeBuffer();
    void GrowBuffer();
    void WriteBits(int value, int bits);
    void SetByteAt(int offset, unsigned char value);
};

#endif
