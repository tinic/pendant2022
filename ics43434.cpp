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

#if 0
#include <stdio.h>

extern "C" {
    extern I2S_HandleTypeDef hi2s2;
    void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *) {
        // next block
        ICS43434::instance().completeDMA();
    }
};

ICS43434 &ICS43434::instance() {
    static ICS43434 ics43434;
    if (!ics43434.initialized) {
        ics43434.initialized = true;
        ics43434.init();
    }
    return ics43434;
}

void ICS43434::init() {
    update();
}

void ICS43434::update() {
    if (HAL_GPIO_ReadPin(VDD30_SW_GPIO_Port, VDD30_SW_Pin) == GPIO_PIN_RESET) {
        startedDMA = false;
    } else {
        startDMA();
    }
}

void ICS43434::completeDMA() {
    if (!startedDMA) {
        return;
    }

    HAL_I2S_Receive_DMA(&hi2s2, (uint16_t *)data.data(), transferSamples);
}

void ICS43434::startDMA() {
    if (startedDMA) {
        return;
    }

    startedDMA = true;

    HAL_I2S_Receive_DMA(&hi2s2, (uint16_t *)data.data(), transferSamples);
}
#endif  // #if 0
