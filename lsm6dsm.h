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
#ifndef _LSM6DSM_
#define _LSM6DSM_

#include <stdint.h>

class LSM6DSM {
public:
    static LSM6DSM &instance();

    void update();

    float temperature() const { return (static_cast<float>(lsm6dsmRegs.fields.outTemp) * (1.0f/256.f)) + 25.0f; }

    float XG() const { return static_cast<float>(lsm6dsmRegs.fields.outXG) * gRes; }
    float YG() const { return static_cast<float>(lsm6dsmRegs.fields.outYG) * gRes; }
    float ZG() const { return static_cast<float>(lsm6dsmRegs.fields.outZG) * gRes; }

    float XA() const { return static_cast<float>(lsm6dsmRegs.fields.outXA) * aRes; }
    float YA() const { return static_cast<float>(lsm6dsmRegs.fields.outYA) * aRes; }
    float ZA() const { return static_cast<float>(lsm6dsmRegs.fields.outZA) * aRes; }

private:

    friend class i2c1;
    friend class i2c2;
    
    static constexpr uint8_t i2c_addr = 0x6B;
    static constexpr const char *str_id = "LSM6DSM";
    static bool devicePresent;

    void read();
    void init();
    void stats();

    void reset();

    enum {
        AFS_2G                          = 0x00,
        AFS_4G                          = 0x02,
        AFS_8G                          = 0x03,
        AFS_16G                         = 0x01,

        GFS_245DPS                      = 0x00,
        GFS_500DPS                      = 0x01,
        GFS_1000DPS                     = 0x02,
        GFS_2000DPS                     = 0x03,

        AODR_12_5Hz                     = 0x01,  // same for accel and gyro in normal mode
        AODR_26Hz                       = 0x02,
        AODR_52Hz                       = 0x03,
        AODR_104Hz                      = 0x04,
        AODR_208Hz                      = 0x05,
        AODR_416Hz                      = 0x06,
        AODR_833Hz                      = 0x07,
        AODR_1660Hz                     = 0x08,
        AODR_3330Hz                     = 0x09,
        AODR_6660Hz                     = 0x0A,

        GODR_12_5Hz                     = 0x01,   
        GODR_26Hz                       = 0x02,
        GODR_52Hz                       = 0x03,
        GODR_104Hz                      = 0x04,
        GODR_208Hz                      = 0x05,
        GODR_416Hz                      = 0x06,
        GODR_833Hz                      = 0x07,
        GODR_1660Hz                     = 0x08,
        GODR_3330Hz                     = 0x09,
        GODR_6660Hz                     = 0x0A,
    };

    union LSM6DSMRegs {
        uint8_t regs[14];
        struct  __attribute__ ((__packed__)) {
            uint16_t outTemp;

            uint16_t outXG;
            uint16_t outYG;
            uint16_t outZG;

            uint16_t outXA;
            uint16_t outYA;
            uint16_t outZA;
        } fields;
    } lsm6dsmRegs = { };

    void config(uint8_t Ascale = AFS_2G, 
                uint8_t Gscale = GFS_245DPS, 
                uint8_t AODR = AODR_208Hz, 
                uint8_t GODR = GODR_416Hz);

    bool initialized = false;
    bool configured = false;
    bool resetted = false;

    uint8_t aScale = 0;
    uint8_t gScale = 0;
    uint8_t aodr = 0;
    uint8_t godr = 0;
    float aRes = 0.0f;
    float gRes = 0.0f;
};

#endif  // #ifndef _LSM6DSM_
