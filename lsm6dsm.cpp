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
#include "./lsm6dsm.h"
#include "./i2cmanager.h"

#include <stdio.h>

enum {
    LSM6DSM_FUNC_CFG_ACCESS          = 0x01,
    LSM6DSM_SENSOR_SYNC_TIME_FRAME   = 0x04,
    LSM6DSM_SENSOR_SYNC_RES_RATIO    = 0x05,
    LSM6DSM_FIFO_CTRL1               = 0x06,
    LSM6DSM_FIFO_CTRL2               = 0x07,
    LSM6DSM_FIFO_CTRL3               = 0x08,
    LSM6DSM_FIFO_CTRL4               = 0x09,
    LSM6DSM_FIFO_CTRL5               = 0x0A,
    LSM6DSM_DRDY_PULSE_CFG           = 0x0B,
    LSM6DSM_INT1_CTRL                = 0x0D,
    LSM6DSM_INT2_CTRL                = 0x0E,
    LSM6DSM_WHO_AM_I                 = 0x0F, // should be 0x6A
    LSM6DSM_CTRL1_XL                 = 0x10,
    LSM6DSM_CTRL2_G                  = 0x11,
    LSM6DSM_CTRL3_C                  = 0x12,
    LSM6DSM_CTRL4_C                  = 0x13,
    LSM6DSM_CTRL5_C                  = 0x14,
    LSM6DSM_CTRL6_C                  = 0x15,
    LSM6DSM_CTRL7_G                  = 0x16,
    LSM6DSM_CTRL8_XL                 = 0x17,
    LSM6DSM_CTRL9_XL                 = 0x18,
    LSM6DSM_CTRL10_C                 = 0x19,
    LSM6DSM_MASTER_CONFIG            = 0x1A,
    LSM6DSM_WAKE_UP_SRC              = 0x1B,
    LSM6DSM_TAP_SRC                  = 0x1C,
    LSM6DSM_D6D_SRC                  = 0x1D,
    LSM6DSM_STATUS_REG               = 0x1E,
    LSM6DSM_OUT_TEMP_L               = 0x20,
    LSM6DSM_OUT_TEMP_H               = 0x21,
    LSM6DSM_OUTX_L_G                 = 0x22,
    LSM6DSM_OUTX_H_G                 = 0x23,
    LSM6DSM_OUTY_L_G                 = 0x24,
    LSM6DSM_OUTY_H_G                 = 0x25,
    LSM6DSM_OUTZ_L_G                 = 0x26,
    LSM6DSM_OUTZ_H_G                 = 0x27,
    LSM6DSM_OUTX_L_XL                = 0x28,
    LSM6DSM_OUTX_H_XL                = 0x29,
    LSM6DSM_OUTY_L_XL                = 0x2A,
    LSM6DSM_OUTY_H_XL                = 0x2B,
    LSM6DSM_OUTZ_L_XL                = 0x2C,
    LSM6DSM_OUTZ_H_XL                = 0x2D,
    LSM6DSM_SENSORHUB1_REG           = 0x2E,
    LSM6DSM_SENSORHUB2_REG           = 0x2F,
    LSM6DSM_SENSORHUB3_REG           = 0x30,
    LSM6DSM_SENSORHUB4_REG           = 0x31,
    LSM6DSM_SENSORHUB5_REG           = 0x32,
    LSM6DSM_SENSORHUB6_REG           = 0x33,
    LSM6DSM_SENSORHUB7_REG           = 0x34,
    LSM6DSM_SENSORHUB8_REG           = 0x35,
    LSM6DSM_SENSORHUB9_REG           = 0x36,
    LSM6DSM_SENSORHUB10_REG          = 0x37,
    LSM6DSM_SENSORHUB11_REG          = 0x38,
    LSM6DSM_SENSORHUB12_REG          = 0x39,
    LSM6DSM_FIFO_STATUS1             = 0x3A,
    LSM6DSM_FIFO_STATUS2             = 0x3B,
    LSM6DSM_FIFO_STATUS3             = 0x3C,
    LSM6DSM_FIFO_STATUS4             = 0x3D,
    LSM6DSM_FIFO_DATA_OUT_L          = 0x3E,
    LSM6DSM_FIFO_DATA_OUT_H          = 0x3F,
    LSM6DSM_TIMESTAMP0_REG           = 0x40,
    LSM6DSM_TIMESTAMP1_REG           = 0x41,
    LSM6DSM_TIMESTAMP2_REG           = 0x42,
    LSM6DSM_STEP_TIMESTAMP_L         = 0x49,
    LSM6DSM_STEP_TIMESTAMP_H         = 0x4A,
    LSM6DSM_STEP_COUNTER_L           = 0x4B,
    LSM6DSM_STEP_COUNTER_H           = 0x4C,
    LSM6DSM_SENSORHUB13_REG          = 0x4D,
    LSM6DSM_SENSORHUB14_REG          = 0x4E,
    LSM6DSM_SENSORHUB15_REG          = 0x4F,
    LSM6DSM_SENSORHUB16_REG          = 0x50,
    LSM6DSM_SENSORHUB17_REG          = 0x51,
    LSM6DSM_SENSORHUB18_REG          = 0x52,
    LSM6DSM_FUNC_SRC1                = 0x53,
    LSM6DSM_FUNC_SRC2                = 0x54,
    LSM6DSM_WRIST_TILT_IA            = 0x55,
    LSM6DSM_TAP_CFG                  = 0x58,
    LSM6DSM_TAP_THS_6D               = 0x59,
    LSM6DSM_INT_DUR2                 = 0x5A,
    LSM6DSM_WAKE_UP_THS              = 0x5B,
    LSM6DSM_WAKE_UP_DUR              = 0x5C,
    LSM6DSM_FREE_FALL                = 0x5D,
    LSM6DSM_MD1_CFG                  = 0x5E,
    LSM6DSM_MD2_CFG                  = 0x5F,
    LSM6DSM_MASTER_MODE_CODE         = 0x60,
    LSM6DSM_SENS_SYNC_SPI_ERROR_CODE = 0x61,
    LSM6DSM_OUT_MAG_RAW_X_L          = 0x66,
    LSM6DSM_OUT_MAG_RAW_X_H          = 0x67,
    LSM6DSM_OUT_MAG_RAW_Y_L          = 0x68,
    LSM6DSM_OUT_MAG_RAW_Y_H          = 0x69,
    LSM6DSM_OUT_MAG_RAW_Z_L          = 0x6A,
    LSM6DSM_OUT_MAG_RAW_Z_H          = 0x6B,
    LSM6DSM_INT_OIS                  = 0x6F,
    LSM6DSM_CTRL1_OIS                = 0x70,
    LSM6DSM_CTRL2_OIS                = 0x71,
    LSM6DSM_CTRL3_OIS                = 0x72,
    LSM6DSM_X_OFS_USR                = 0x73,
    LSM6DSM_Y_OFS_USR                = 0x74,
    LSM6DSM_Z_OFS_USR                = 0x75
};

static void delay_us(int usec) {
    asm volatile (
        "1:\n\t"
#define NOP6 "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
#define NOP24 NOP6 NOP6 NOP6 NOP6
#define NOP96 NOP24 NOP24 NOP24 NOP24
        NOP96
        "subs %[usec], #1\n\t"
        "bge 1b\n\t"
    : : [usec] "r" (usec) : );
}

bool LSM6DSM::devicePresent = false;

LSM6DSM &LSM6DSM::instance() {
    static LSM6DSM lsm6dsm;
    if (!lsm6dsm.initialized) {
        lsm6dsm.initialized = true;
        lsm6dsm.init();
    }
    return lsm6dsm;
}

void LSM6DSM::update() {
    if (!devicePresent) return;
    reset();
    config();
    read();
}

void LSM6DSM::init() {
    if (!devicePresent) return;
    update();
    stats();
}

void LSM6DSM::reset() {
    if (resetted) {
        return;
    }

    resetted = true;

    uint8_t temp = i2c1::instance().getReg8(i2c_addr, LSM6DSM_CTRL3_C);
    i2c1::instance().setReg8(i2c_addr, LSM6DSM_CTRL3_C, temp | 0x01); // Set bit 0 to 1 to reset LSM6DSM
    delay_us(100); // Wait for all registers to reset 
}

void LSM6DSM::config(uint8_t _aScale, uint8_t _gScale, uint8_t _aodr, uint8_t _godr) {

    if (_aScale != aScale ||
        _gScale != gScale ||
        _aodr   != aodr ||
        _godr   != godr) {
        configured = false;
    }

    if (configured) {
        return;
    }

    configured = true;

    aScale = _aScale;
    gScale = _gScale;
    aodr   = _aodr;
    godr   = _godr;

    switch (aScale)
    {
        default:
        case AFS_2G:
            aRes = 2.0f/32768.0f;
            break;
        case AFS_4G:
            aRes = 4.0f/32768.0f;
            break;
        case AFS_8G:
            aRes = 8.0f/32768.0f;
            break;
        case AFS_16G:
            aRes = 16.0f/32768.0f;
            break;
    }

    switch (gScale)
    {
        default:
        case GFS_245DPS:
            gRes = 245.0f/32768.0f;
            break;
        case GFS_500DPS:
            gRes = 500.0f/32768.0f;
            break;
        case GFS_1000DPS:
            gRes = 1000.0f/32768.0f;
            break;
        case GFS_2000DPS:
            gRes = 2000.0f/32768.0f;
            break;
    }

    i2c1::instance().setReg8(i2c_addr, LSM6DSM_CTRL1_XL, aodr << 4 | aScale << 2);
    i2c1::instance().setReg8(i2c_addr, LSM6DSM_CTRL2_G, godr << 4 | gScale << 2);

    // enable block update (bit 6 = 1), auto-increment registers (bit 2 = 1)
    i2c1::instance().setReg8(i2c_addr, LSM6DSM_CTRL3_C, i2c1::instance().getReg8(i2c_addr, LSM6DSM_CTRL3_C) | 0x40 | 0x04); 

    // by default, interrupts active HIGH, push pull, little endian data 
    // (can be changed by writing to bits 5, 4, and 1, resp to above register)

    // enable accel LP2 (bit 7 = 1), set LP2 tp ODR/9 (bit 6 = 1), enable input_composite (bit 3) for low noise
    i2c1::instance().setReg8(i2c_addr, LSM6DSM_CTRL8_XL, 0x80 | 0x40 | 0x08 );

    // interrupt handling
    i2c1::instance().setReg8(i2c_addr, LSM6DSM_DRDY_PULSE_CFG, 0x80); // latch interrupt until data read
    i2c1::instance().setReg8(i2c_addr, LSM6DSM_INT1_CTRL, 0x43);      // enable significant motion interrupts 
                                                                      // and accel/gyro data ready interrupts on INT1  

    //i2c1::instance().setReg8(i2c_addr, LSM6DSM_CTRL4_C, 0x40);
}

void LSM6DSM::read() {
    uint8_t startReg = LSM6DSM_OUT_TEMP_L;
    i2c1::instance().writeRead(i2c_addr, &startReg, sizeof(startReg), (uint8_t *)&lsm6dsmRegs, sizeof(lsm6dsmRegs));
}

void LSM6DSM::stats() {
    printf("LSM6DSM temp %fC\r\n", double(temperature()));
    printf("LSM6DSM gyro (X:%f Y:%f Z:%f)\r\n", double(XG()), double(YG()), double(ZG()));
    printf("LSM6DSM accel (X:%f Y:%f Z:%f)\r\n", double(XA()), double(YA()), double(ZA()));
}
