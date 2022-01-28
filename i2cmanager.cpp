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
#include "./i2cmanager.h"
#include "./sdd1306.h"
#include "./timeline.h"
#include "./bq25895.h"
#include "./ens210.h"
#include "./lsm6dsm.h"
#include "./mmc5633njl.h"

#include "M480.h"

#include <memory.h>

template<typename T> void i2c1::checkReady() {
    if (!T::devicePresent) {
        T::devicePresent = I2C_WriteByte(I2C2, T::i2c_addr, 0) == 0;
        if (T::devicePresent) printf("%s is ready.\r\n", T::str_id);
    }
}

template<class T> void i2c1::checkReadyReprobe() {
    if (!T::devicePresent) {
        T::devicePresent = I2C_WriteByte(I2C2, T::i2c_addr, 0) == 0;
        if (T::devicePresent) printf("%s is is ready on reprobe.\r\n", T::str_id);
    }
}

template<class T> void i2c1::update() {
    if (T::devicePresent) {
        T::instance().update();
    }
}

i2c1 &i2c1::instance() {
    static i2c1 i2c;
    if (!i2c.initialized) {
        i2c.initialized = true;
        i2c.init();
    }
    return i2c;
}

void i2c1::init() {

    GPIO_SetMode(PA, BIT6, GPIO_MODE_OPEN_DRAIN);
    GPIO_SetMode(PA, BIT7, GPIO_MODE_OPEN_DRAIN);

    PA6 = 1;
    PA7 = 1;

    PA->SMTEN |= GPIO_SMTEN_SMTEN6_Msk | GPIO_SMTEN_SMTEN7_Msk;

    I2C_Open(I2C1, 400000);

    uint32_t STCTL = 0;
    uint32_t HTCTL = 2;

    I2C1->TMCTL = ( ( STCTL << I2C_TMCTL_STCTL_Pos) & I2C_TMCTL_STCTL_Msk ) |
                  ( ( HTCTL << I2C_TMCTL_HTCTL_Pos) & I2C_TMCTL_HTCTL_Msk );

    checkReady<BQ25895>();
    checkReady<ENS210>();
    checkReady<LSM6DSM>();
    checkReady<MMC5633NJL>();
}

void i2c1::update() {
    checkReadyReprobe<BQ25895>();
    checkReadyReprobe<ENS210>();
    checkReadyReprobe<LSM6DSM>();
    checkReadyReprobe<MMC5633NJL>();

    update<BQ25895>();
    update<ENS210>();
    update<LSM6DSM>();
    update<MMC5633NJL>();
}

void i2c1::write(uint8_t _u8PeripheralAddr, uint8_t data[], size_t _u32wLen) {
    I2C_WriteMultiBytes(I2C1, _u8PeripheralAddr, data, _u32wLen);
}

uint32_t i2c1::read(uint8_t _u8PeripheralAddr, uint8_t rdata[], size_t _u32rLen) {
    return I2C_ReadMultiBytes(I2C1, _u8PeripheralAddr, rdata, _u32rLen);
}

uint32_t i2c1::writeRead(uint8_t _u8PeripheralAddr, uint8_t data[], size_t _u32wLen, uint8_t rdata[], size_t _u32rLen) {
    I2C_WriteMultiBytes(I2C1, _u8PeripheralAddr, data, _u32wLen);
    return I2C_ReadMultiBytes(I2C1, _u8PeripheralAddr, rdata, _u32rLen);
}

uint8_t i2c1::getReg8(uint8_t _u8PeripheralAddr, uint8_t _u8DataAddr) {
    return I2C_ReadByteOneReg(I2C1, _u8PeripheralAddr, _u8DataAddr);
}

void i2c1::setReg8(uint8_t _u8PeripheralAddr, uint8_t _u8DataAddr, uint8_t _u8WData) {
    I2C_WriteByteOneReg(I2C1, _u8PeripheralAddr, _u8DataAddr, _u8WData);
}

void i2c1::setReg8Bits(uint8_t peripheralAddr, uint8_t reg, uint8_t mask) {
    uint8_t value = getReg8(peripheralAddr, reg);
    value |= mask;
    setReg8(peripheralAddr, reg, value);
}

void i2c1::clearReg8Bits(uint8_t peripheralAddr, uint8_t reg, uint8_t mask) {
    uint8_t value = getReg8(peripheralAddr, reg);
    value &= uint8_t(~mask);
    setReg8(peripheralAddr, reg, value);
}

extern "C" {
    void I2C2_IRQHandler(void) {
        i2c2::instance().I2C2_IRQHandler();
    }
    void PDMA_IRQHandler(void) {
        i2c2::instance().PDMA_IRQHandler();
    }
}

template<typename T> void i2c2::checkReady() {
    if (!T::devicePresent) {
        T::devicePresent = I2C_WriteByte(I2C2, T::i2c_addr, 0) == 0;
        if (T::devicePresent) printf("%s is ready.\r\n", T::str_id);
    }
}

template<class T> void i2c2::checkReadyReprobe() {
    if (!T::devicePresent) {
        T::devicePresent = I2C_WriteByte(I2C2, T::i2c_addr, 0) == 0;
        if (T::devicePresent) printf("%s is is ready on reprobe.\r\n", T::str_id);
    }
}

template<class T> void i2c2::update() {
    if (T::devicePresent) {
        T::instance().update();
    }
}

i2c2 &i2c2::instance() {
    static i2c2 i2c;
    if (!i2c.initialized) {
        i2c.initialized = true;
        i2c.init();
    }
    return i2c;
}

void i2c2::init() {

    GPIO_SetMode(PA, BIT10, GPIO_MODE_OPEN_DRAIN);
    GPIO_SetMode(PA, BIT11, GPIO_MODE_OPEN_DRAIN);

    PA10 = 1;
    PA11 = 1;

    PA->SMTEN |= GPIO_SMTEN_SMTEN10_Msk | GPIO_SMTEN_SMTEN11_Msk;

    PDMA_Open(PDMA, 1 << I2C2_PDMA_TX_CH);

    PDMA_EnableInt(PDMA, I2C2_PDMA_TX_CH, 0);
    PDMA_SetBurstType(PDMA, I2C2_PDMA_TX_CH, PDMA_REQ_SINGLE, 0);

    I2C_Open(I2C2, 600000);

    uint32_t STCTL = 0;
    uint32_t HTCTL = 2;

    I2C2->TMCTL = ( ( STCTL << I2C_TMCTL_STCTL_Pos) & I2C_TMCTL_STCTL_Msk ) |
                  ( ( HTCTL << I2C_TMCTL_HTCTL_Pos) & I2C_TMCTL_HTCTL_Msk );

    NVIC_SetPriority(PDMA_IRQn, 3);
    NVIC_EnableIRQ(PDMA_IRQn);

    NVIC_SetPriority(I2C2_IRQn, 3);
    NVIC_EnableIRQ(I2C2_IRQn);

    checkReady<SDD1306>();
}

void i2c2::update() {
    checkReadyReprobe<SDD1306>();
}

void i2c2::prepareBatchWrite() {
    qBufEnd = qBufSeq;
    qBufPtr = qBufSeq;
}

void i2c2::queueBatchWrite(uint8_t peripheralAddr, uint8_t data[], size_t len) {

    if (!qBufEnd || !qBufPtr) {
        printf("Not in batch mode!\n");
        return;
    }

    if (size_t((qBufEnd + len + 2) - qBufSeq) > sizeof(qBufSeq)) {
        printf("I2C batch buffer too small (wanted at least %d bytes)!\n", int(((qBufEnd + len + 2) - qBufSeq)));
        return;
    }

    if (len > 255) {
        printf("i2c2::queueBatchWrite len overflow!\n");
        return;
    }

    *qBufEnd++ = uint8_t(len);
    *qBufEnd++ = peripheralAddr << 1;
    memcpy(qBufEnd, data, len);
    qBufEnd += len;
}

void i2c2::I2C2_IRQHandler(void) {
    if (I2C_GET_TIMEOUT_FLAG(I2C2)) {
        I2C_ClearTimeoutFlag(I2C2);
    } else {
        uint32_t u32Status = I2C_GET_STATUS(I2C2);
        if(u32Status == 0x08) {        /* START has been transmitted */
        } else if(u32Status == 0x10) { /* Repeat START has been transmitted */
        } else if(u32Status == 0x18) { /* SLA+W has been transmitted and ACK has been received */
        } else if(u32Status == 0x20) { /* SLA+W has been transmitted and NACK has been received */
            I2C_STOP(I2C2);
            I2C_START(I2C2);
        } else if(u32Status == 0x28) { /* DATA has been transmitted and ACK has been received */
        } else {
        }
    }
}

void i2c2::PDMA_IRQHandler(void) {
    uint32_t u32Status = PDMA->TDSTS;
    if(u32Status & (0x1 << I2C2_PDMA_TX_CH)) {
        PDMA->TDSTS = 0x1 << I2C2_PDMA_TX_CH;
        pdmaDone = true;
    }
    if(u32Status & (0x1 << 2)) {
        PDMA->TDSTS = 0x1 << 2;
        printf("2!!!\n");
    }
    if(u32Status & (0x1 << 3)) {
        PDMA->TDSTS = 0x1 << 3;
        printf("2!!!\n");
    }
}

void i2c2::performBatchWrite() {

    if (!qBufEnd || !qBufPtr) {
        printf("Not in batch mode!\n");
        return;
    }

    I2C_EnableInt(I2C2);
    I2C_ENABLE_TX_PDMA(I2C2);
    I2C_DISABLE_RX_PDMA(I2C2);

    for (; qBufPtr < qBufEnd;) {
        pdmaDone = false;
        uint8_t u32wLen = *qBufPtr++;
        PDMA_SetTransferMode(PDMA, I2C2_PDMA_TX_CH, PDMA_I2C2_TX, 0, 0);
        PDMA_SetTransferCnt(PDMA, I2C2_PDMA_TX_CH, PDMA_WIDTH_8, u32wLen + 1);
        PDMA_SetTransferAddr(PDMA, I2C2_PDMA_TX_CH, ((uint32_t) (&qBufPtr[0])), PDMA_SAR_INC, (uint32_t)(&(I2C2->DAT)), PDMA_DAR_FIX);
        I2C_START(I2C2);
        for (; !pdmaDone ;) { __WFI(); }
        for (; (I2C2->STATUS1 & I2C_STATUS1_ONBUSY_Msk) != 0 ;) { }
        qBufPtr += u32wLen + 1;
    }
    
    I2C_DISABLE_TX_PDMA(I2C2);
    I2C_DISABLE_RX_PDMA(I2C2);
    I2C_DisableInt(I2C2);
}

void i2c2::write(uint8_t _u8PeripheralAddr, uint8_t data[], size_t _u32wLen) {
    I2C_WriteMultiBytes(I2C2, _u8PeripheralAddr, data, _u32wLen);
}

uint32_t i2c2::read(uint8_t _u8PeripheralAddr, uint8_t rdata[], size_t _u32rLen) {
    return I2C_ReadMultiBytes(I2C2, _u8PeripheralAddr, rdata, _u32rLen);
}

uint8_t i2c2::getReg8(uint8_t _u8PeripheralAddr, uint8_t _u8DataAddr) {
    return I2C_ReadByteOneReg(I2C2, _u8PeripheralAddr, _u8DataAddr);
}

void i2c2::setReg8(uint8_t _u8PeripheralAddr, uint8_t _u8DataAddr, uint8_t _u8WData) {
    I2C_WriteByteOneReg(I2C2, _u8PeripheralAddr, _u8DataAddr, _u8WData);
}

void i2c2::setReg8Bits(uint8_t peripheralAddr, uint8_t reg, uint8_t mask) {
    uint8_t value = getReg8(peripheralAddr, reg);
    value |= mask;
    setReg8(peripheralAddr, reg, value);
}

void i2c2::clearReg8Bits(uint8_t peripheralAddr, uint8_t reg, uint8_t mask) {
    uint8_t value = getReg8(peripheralAddr, reg);
    value &= uint8_t(~mask);
    setReg8(peripheralAddr, reg, value);
}
