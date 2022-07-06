/*
Copyright 2021 Tinic Uro

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
#ifndef _MMC5633NJL_
#define _MMC5633NJL_

#include <stdint.h>

class MMC5633NJL {
public:
    static MMC5633NJL &instance();

    void update();

    float X() const;
    float Y() const;
    float Z() const;
    float temperature() const;

    uint32_t XGRaw() const;
    uint32_t YGRaw() const;
    uint32_t ZGRaw() const;
    uint8_t temperatureRaw() const;

private:

    friend class i2c1;
    friend class i2c2;

    static constexpr uint8_t i2c_addr = 0x30;
    static constexpr const char *str_id = "MMC5633NJL";
    static bool devicePresent;

    void read();
    void init();
    void stats();
    void status();
    void readTemp();
    void readAccel();

    void reset();

    union MMC5633NJLRegs {
        uint8_t regs[10]; // RDLONG
        struct  __attribute__ ((__packed__)) {
            // RDSHORT, 6 bytes
            uint8_t Xout0;
            uint8_t Xout1;

            uint8_t Yout0;
            uint8_t Yout1;

            uint8_t Zout0;
            uint8_t Zout1;

            // RDLONG, 6 + 3 + 1 bytes
            uint8_t Xout2;
            uint8_t Yout2;
            uint8_t Zout2;

            uint8_t Tout;
        } fields;
    } mmc5633njlRegs = { };

    void config();

    bool initialized = false;
    bool configured = false;
    bool resetted = false;
};

#endif  // #ifndef _MMC5633NJL_
