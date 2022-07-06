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
#include "./mmc5633njl.h"
#include "./i2cmanager.h"

#include <stdio.h>
#include <memory.h>

enum {
    // Registers
    MMC5633NJL_REG_XOUT0                    = 0x00,
    MMC5633NJL_REG_XOUT1                    = 0x01,
    MMC5633NJL_REG_YOUT0                    = 0x02,
    MMC5633NJL_REG_YOUT1                    = 0x03,
    MMC5633NJL_REG_ZOUT0                    = 0x04,
    MMC5633NJL_REG_ZOUT1                    = 0x05,
    MMC5633NJL_REG_XOUT2                    = 0x06,
    MMC5633NJL_REG_YOUT2                    = 0x07,
    MMC5633NJL_REG_ZOUT2                    = 0x08,

    MMC5633NJL_REG_TOUT                     = 0x09,
    MMC5633NJL_REG_TPH0                     = 0x0A,
    MMC5633NJL_REG_TPH1                     = 0x0B,
    MMC5633NJL_REG_TU                       = 0x0C,

    MMC5633NJL_REG_STATUS_0                 = 0x19,
    MMC5633NJL_REG_STATUS_1                 = 0x18,
    MMC5633NJL_REG_ODR                      = 0x1A,
    MMC5633NJL_REG_CTRL_0                   = 0x1B,
    MMC5633NJL_REG_CTRL_1                   = 0x1C,
    MMC5633NJL_REG_CTRL_2                   = 0x1D,
    MMC5633NJL_REG_ST_X_TH                  = 0x1E,
    MMC5633NJL_REG_ST_Y_TH                  = 0x1F,
    MMC5633NJL_REG_ST_Z_TH                  = 0x20,
    MMC5633NJL_REG_ST_X                     = 0x27,
    MMC5633NJL_REG_ST_Y                     = 0x28,
    MMC5633NJL_REG_ST_Z                     = 0x29,
    MMC5633NJL_REG_PRODUCT_ID               = 0x39,

    // Status bits
    MMC5633NJL_STATUS_0_INT_MASK            = 0b00000011,
    MMC5633NJL_STATUS_0_INT_NONE            = 0b00000000,
    MMC5633NJL_STATUS_0_INT_MEAS_MT_DONE    = 0b00000001,
    MMC5633NJL_STATUS_0_INT_MDT_FLAG_HIGH   = 0b00000010,
    MMC5633NJL_STATUS_0_INT_PROCOTOL_ERROR  = 0b00000011,

    MMC5633NJL_STATUS_0_MDT_FLAG            = 0b00000100,
    MMC5633NJL_STATUS_0_PROCOTOL_ERROR      = 0b00100000,

    MMC5633NJL_STATUS_0_ACTIVTY_MASK        = 0b11000000,
    MMC5633NJL_STATUS_0_ACTIVTY_POWER_DOWN  = 0b00000000,
    MMC5633NJL_STATUS_0_ACTIVTY_RESET       = 0b01000000,
    MMC5633NJL_STATUS_0_ACTIVTY_MEASUREMENT = 0b10000000,

    MMC5633NJL_STATUS_1_MEAS_M_DONE_INT     = 0b00000001,
    MMC5633NJL_STATUS_1_MEAS_T_DONE_INT     = 0b00000010,
    MMC5633NJL_STATUS_1_MDT_FLAG_INT        = 0b00000100,
    MMC5633NJL_STATUS_1_ST_FAIL             = 0b00001000,
    MMC5633NJL_STATUS_1_OTP_READ_DONE       = 0b00010000,
    MMC5633NJL_STATUS_1_SAT_SENSOR          = 0b00100000,
    MMC5633NJL_STATUS_1_MEAS_M_DONE         = 0b01000000,
    MMC5633NJL_STATUS_1_MEAS_T_DONE         = 0b10000000,

    // Control bits
    MMC5633NJL_CTRL_0_TAKE_MEAS_M           = 0b00000001,
    MMC5633NJL_CTRL_0_TAKE_MEAS_T           = 0b00000010,
    MMC5633NJL_CTRL_0_START_MDT             = 0b00000100,
    MMC5633NJL_CTRL_0_DO_SET                = 0b00001000,
    MMC5633NJL_CTRL_0_DO_RESET              = 0b00010000,
    MMC5633NJL_CTRL_0_AUTO_SR_EN            = 0b00100000,
    MMC5633NJL_CTRL_0_AUTO_ST_EN            = 0b01000000,
    MMC5633NJL_CTRL_0_CMM_FREQ_EN           = 0b10000000,

    MMC5633NJL_CTRL_1_BW_MASK               = 0b00000011,
    MMC5633NJL_CTRL_1_BW_6_6_MS             = 0b00000000,
    MMC5633NJL_CTRL_1_BW_3_5_MS             = 0b00000001,
    MMC5633NJL_CTRL_1_BW_2_0_MS             = 0b00000010,
    MMC5633NJL_CTRL_1_BW_1_2_MS             = 0b00000011,

    MMC5633NJL_CTRL_1_X_INHIBIT             = 0b00000100,
    MMC5633NJL_CTRL_1_Y_INHIBIT             = 0b00001000,
    MMC5633NJL_CTRL_1_Z_INHIBIT             = 0b00010000,
    MMC5633NJL_CTRL_1_ST_ENP                = 0b00100000,
    MMC5633NJL_CTRL_1_ST_ENM                = 0b01000000,
    MMC5633NJL_CTRL_1_SW_RESET              = 0b10000000,

    MMC5633NJL_CTRL_2_PRD_SET_MASK          = 0b00000111,
    MMC5633NJL_CTRL_2_PRD_SET_1_MPS         = 0b00000000,
    MMC5633NJL_CTRL_2_PRD_SET_25_MPS        = 0b00000001,
    MMC5633NJL_CTRL_2_PRD_SET_75_MPS        = 0b00000010,
    MMC5633NJL_CTRL_2_PRD_SET_100_MPS       = 0b00000011,
    MMC5633NJL_CTRL_2_PRD_SET_250_MPS       = 0b00000100,
    MMC5633NJL_CTRL_2_PRD_SET_500_MPS       = 0b00000101,
    MMC5633NJL_CTRL_2_PRD_SET_1000_MPS      = 0b00000110,
    MMC5633NJL_CTRL_2_PRD_SET_2000_MPS      = 0b00000111,

    MMC5633NJL_CTRL_2_EN_PRD_SET            = 0b00001000,
    MMC5633NJL_CTRL_2_COMM_EN               = 0b00010000,
    MMC5633NJL_CTRL_2_INT_MDT_EN            = 0b00100000,
    MMC5633NJL_CTRL_2_INT_MEAS_DONE_EN      = 0b01000000,
    MMC5633NJL_CTRL_2_HPOWER                = 0b10000000,
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

bool MMC5633NJL::devicePresent = false;

MMC5633NJL &MMC5633NJL::instance() {
    static MMC5633NJL lsm6dsm;
    if (!lsm6dsm.initialized) {
        lsm6dsm.initialized = true;
        lsm6dsm.init();
    }
    return lsm6dsm;
}

void MMC5633NJL::update() {
    if (!devicePresent) return;
    reset();
    config();
    read();
}

void MMC5633NJL::status() {
    uint8_t status0 = i2c1::instance().getReg8(i2c_addr, MMC5633NJL_REG_STATUS_0);
    printf("st0 %02x\n", status0);
    uint8_t status1 = i2c1::instance().getReg8(i2c_addr, MMC5633NJL_REG_STATUS_1);
    printf("st1 %02x\n", status1);
}

void MMC5633NJL::init() {
    if (!devicePresent) return;

    i2c1::instance().setReg8(i2c_addr, MMC5633NJL_REG_CTRL_1, MMC5633NJL_CTRL_1_SW_RESET);

    delay_us(20);

    for ( ; ; ) 
    {
        // Wait for power down
        uint8_t status1 = i2c1::instance().getReg8(i2c_addr, MMC5633NJL_REG_STATUS_0);
        if ( ( status1 & ( MMC5633NJL_STATUS_0_ACTIVTY_MASK ) ) == 
                         ( MMC5633NJL_STATUS_0_ACTIVTY_POWER_DOWN ) ) {
            break;
        }
        delay_us(20);
    }

    update();
    stats();
}

void MMC5633NJL::reset() {
    if (resetted) {
        return;
    }

    resetted = true;

    // NOP for now
}

void MMC5633NJL::config() {
    // NOP for now
}

void MMC5633NJL::readTemp() {
    // Clear old data
    memset(&mmc5633njlRegs.regs, 0, sizeof(mmc5633njlRegs));

    // Measure temperature
    i2c1::instance().setReg8(i2c_addr, MMC5633NJL_REG_CTRL_0, 
        MMC5633NJL_CTRL_0_TAKE_MEAS_T | MMC5633NJL_CTRL_0_AUTO_SR_EN);

    for ( ; ; ) {
        // Wait for measurement
        uint8_t status1 = i2c1::instance().getReg8(i2c_addr, MMC5633NJL_REG_STATUS_1);
        if ( ( status1 & ( MMC5633NJL_STATUS_1_MEAS_T_DONE ) ) == 
                         ( MMC5633NJL_STATUS_1_MEAS_T_DONE ) ) {
            break;
        }
        delay_us(10);
    }

    uint8_t *regs = (uint8_t *)&mmc5633njlRegs.regs;
    // Set start offset for read
    for (size_t c = 0; c < sizeof(mmc5633njlRegs); c++) {
        uint8_t val = i2c1::instance().getReg8(i2c_addr, uint8_t(MMC5633NJL_REG_XOUT0 + c));
        *regs++ = val;
    }
}

void MMC5633NJL::readAccel() {
    // Clear old data
    memset(&mmc5633njlRegs.regs, 0, sizeof(mmc5633njlRegs));

    // Measure temperature
    i2c1::instance().setReg8(i2c_addr, MMC5633NJL_REG_CTRL_0, 
        MMC5633NJL_CTRL_0_TAKE_MEAS_M | MMC5633NJL_CTRL_0_AUTO_SR_EN);

    for ( ; ; ) {
        // Wait for measurement
        uint8_t status1 = i2c1::instance().getReg8(i2c_addr, MMC5633NJL_REG_STATUS_1);
        if ( ( status1 & ( MMC5633NJL_STATUS_1_MEAS_M_DONE ) ) == 
                         ( MMC5633NJL_STATUS_1_MEAS_M_DONE ) ) {
            break;
        }
        delay_us(10);
    }

    uint8_t *regs = (uint8_t *)&mmc5633njlRegs.regs;
    // Set start offset for read
    for (size_t c = 0; c < sizeof(mmc5633njlRegs); c++) {
        uint8_t val = i2c1::instance().getReg8(i2c_addr, uint8_t(MMC5633NJL_REG_XOUT0 + c));
        *regs++ = val;
    }
}

void MMC5633NJL::read() {
    readTemp();
    readAccel();
}

float MMC5633NJL::X() const {
    return float(
        (uint32_t(mmc5633njlRegs.fields.Xout0) << 12)|
        (uint32_t(mmc5633njlRegs.fields.Xout1) <<  4)|
        (uint32_t(mmc5633njlRegs.fields.Xout2) >>  4)) * (1.0f/16.0f) - 32768.0f;
}

float MMC5633NJL::Y() const {
    return float(
        (uint32_t(mmc5633njlRegs.fields.Yout0) << 12)|
        (uint32_t(mmc5633njlRegs.fields.Yout1) <<  4)|
        (uint32_t(mmc5633njlRegs.fields.Yout2) >>  4)) * (1.0f/16.0f) - 32768.0f;
}

float MMC5633NJL::Z() const {
    return float(
        (uint32_t(mmc5633njlRegs.fields.Zout0) << 12)|
        (uint32_t(mmc5633njlRegs.fields.Zout1) <<  4)|
        (uint32_t(mmc5633njlRegs.fields.Zout2) >>  4)) * (1.0f/16.0f) - 32768.0f;
}

float MMC5633NJL::temperature() const {
    return (float(mmc5633njlRegs.fields.Tout) * (1.0f/1.25f)) - 75.0f;
}

uint32_t MMC5633NJL::XGRaw() const {
    return (
        (uint32_t(mmc5633njlRegs.fields.Xout0) << 12)|
        (uint32_t(mmc5633njlRegs.fields.Xout1) <<  4)|
        (uint32_t(mmc5633njlRegs.fields.Xout2) >>  4));
}

uint32_t MMC5633NJL::YGRaw() const {
    return (
        (uint32_t(mmc5633njlRegs.fields.Yout0) << 12)|
        (uint32_t(mmc5633njlRegs.fields.Yout1) <<  4)|
        (uint32_t(mmc5633njlRegs.fields.Yout2) >>  4));
}

uint32_t MMC5633NJL::ZGRaw() const {
    return (
        (uint32_t(mmc5633njlRegs.fields.Zout0) << 12)|
        (uint32_t(mmc5633njlRegs.fields.Zout1) <<  4)|
        (uint32_t(mmc5633njlRegs.fields.Zout2) >>  4));
}

uint8_t MMC5633NJL::temperatureRaw() const {
    return (mmc5633njlRegs.fields.Tout);
}

void MMC5633NJL::stats() {
    printf("MMC5633NJL (temperature: %fC)\r\n", double(temperature()));
    printf("MMC5633NJL (X: %fG) (Y: %fG) (Z: %fG)\r\n", double(X()),  double(Y()), double(Z()));
}
