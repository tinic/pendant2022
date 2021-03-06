/*
Copyright 2020 Tinic Uro

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef I2CMANAGER_H_
#define I2CMANAGER_H_

#include "./color.h"

class i2c1 {
public:
    static i2c1 &instance();

    i2c1() {}

    void write(uint8_t peripheralAddr, uint8_t data[], size_t len);
    uint32_t read(uint8_t peripheralAddr, uint8_t data[], size_t len);

    uint32_t writeRead(uint8_t peripheralAddr, uint8_t writeData[], size_t writeLen, uint8_t readData[], size_t readLen);

    void setReg8(uint8_t peripheralAddr, uint8_t reg, uint8_t dat);
    uint8_t getReg8(uint8_t peripheralAddr, uint8_t reg);

    void setReg8Bits(uint8_t peripheralAddr, uint8_t reg, uint8_t mask);
    void clearReg8Bits(uint8_t peripheralAddr, uint8_t reg, uint8_t mask);

    void update();

private:
    template<typename T> void checkReady();
    template<typename T> void checkReadyReprobe();
    template<class T> void update();

    void init();

    bool initialized = false;
};


class i2c2 {
public:
    static i2c2 &instance();

    i2c2() {}

    void prepareBatchWrite();
    void queueBatchWrite(uint8_t peripheralAddr, uint8_t data[], size_t len);
    void performBatchWrite();

    void write(uint8_t peripheralAddr, uint8_t data[], size_t len);
    uint32_t read(uint8_t peripheralAddr, uint8_t data[], size_t len);

    void setReg8(uint8_t peripheralAddr, uint8_t reg, uint8_t dat);
    uint8_t getReg8(uint8_t peripheralAddr, uint8_t reg);

    void setReg8Bits(uint8_t peripheralAddr, uint8_t reg, uint8_t mask);
    void clearReg8Bits(uint8_t peripheralAddr, uint8_t reg, uint8_t mask);

    void update();

    void I2C2_IRQHandler();
    void PDMA_IRQHandler();

private:
    template<typename T> void checkReady();
    template<typename T> void checkReadyReprobe();
    template<class T> void update();

    void init();

    bool initialized = false;

    static constexpr uint32_t I2C2_PDMA_TX_CH = 2;

    uint8_t qBufSeq[2048] = { };
    uint8_t *qBufPtr = 0;
    uint8_t *qBufEnd = 0;
    
    volatile bool pdmaDone = false;

};

#endif /* I2CMANAGER_H_ */
