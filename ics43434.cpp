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
#include "./ics43434.h"
#include "./main.h"

#include "M480.h"

#include <stdio.h>

static constexpr size_t buffLen = 4096;
static uint32_t PcmBuff[buffLen] = {0};
static uint32_t volatile u32BuffPos = 0;

extern "C" {

void I2S0_IRQHandler(void) {
    uint32_t u32Reg = I2S_GET_INT_FLAG(I2S0, I2S_STATUS0_TXTHIF_Msk | I2S_STATUS0_RXTHIF_Msk);
    if (u32Reg & I2S_STATUS0_RXTHIF_Msk) {
        if (u32BuffPos < (buffLen-8)) {
            uint32_t *pBuffRx = &PcmBuff[u32BuffPos];
            size_t u32Len = I2S_GET_RX_FIFO_LEVEL(I2S0);
            for (size_t i = 0; i < u32Len; i++ ) {
                pBuffRx[i] = I2S_READ_RX_FIFO(I2S0);
            }
            u32BuffPos += u32Len;
            if (u32BuffPos >= buffLen) {
                u32BuffPos =    0;
            }
        }
    }
}

}

ICS43434 &ICS43434::instance() {
    static ICS43434 ics43434;
    if (!ics43434.initialized) {
        ics43434.initialized = true;
        ics43434.init();
    }
    return ics43434;
}

void ICS43434::init() {
    I2S_Open(I2S0, I2S_MODE_SLAVE, 16000, I2S_DATABIT_24, I2S_ENABLE_MONO, I2S_FORMAT_I2S);
    NVIC_EnableIRQ(I2S0_IRQn);

    I2S_EnableMCLK(I2S0, 12000000);
    I2S_EnableInt(I2S0, I2S_IEN_RXTHIEN_Msk);
    I2S_ENABLE_RX(I2S0);
}

void ICS43434::update() {
}
