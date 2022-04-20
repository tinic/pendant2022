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
#include "./seed.h"

#include "main.h"
#include "M480.h"
#include "trng.h"

static uint32_t g_u32AdcIntFlag = 0;

extern "C" {

void EADC00_IRQHandler(void) {
    g_u32AdcIntFlag = 1;
    EADC_CLR_INT_FLAG(EADC, EADC_STATUS2_ADIF0_Msk);      /* Clear the A/D ADINT0 interrupt flag */
}

}

Seed &Seed::instance() {
    static Seed seed;
    if (!seed.initialized) {
        seed.initialized = true;
        seed.init();
    }
    return seed;
}

void Seed::init() {

    g_u32AdcIntFlag = 0;

    CLK_EnableModuleClock(EADC_MODULE);
    CLK_SetModuleClock(EADC_MODULE, 0, CLK_CLKDIV0_EADC(8));

    PB->MODE &= ~(GPIO_MODE_MODE12_Msk);

    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_EADC0_CH12);

    EADC_Open(EADC, EADC_CTL_DIFFEN_SINGLE_END);
    EADC_ConfigSampleModule(EADC, 0, EADC_ADINT0_TRIGGER, 12);

    EADC_CLR_INT_FLAG(EADC, EADC_STATUS2_ADIF0_Msk);

    EADC_ENABLE_INT(EADC, BIT0);
    NVIC_EnableIRQ(EADC00_IRQn);

    uint32_t h = 0;

    for (size_t c = 0; c < 8; c++) {

        for (volatile int32_t d = 0; d < (1<<16); d++ ) { __NOP(); }

        EADC_ENABLE_SAMPLE_MODULE_INT(EADC, 0, BIT0);

        EADC_START_CONV(EADC, BIT0);

        __WFI();

        EADC_DISABLE_SAMPLE_MODULE_INT(EADC, 0, BIT0);

        while(EADC_GET_DATA_VALID_FLAG(EADC, BIT0) != BIT0);

        uint32_t k = int(EADC_GET_CONV_DATA(EADC, 0));

        k *= 0xcc9e2d51;
        k = (k << 15) | (k >> 17);
        k *= 0x1b873593;

        h ^= k;
        h = (h << 13) | (h >> 19);
        h = h * 5 + 0xe6546b64;
    }

    _seed = h;

    printf("Starting Seed: %08x\n", _seed);

    NVIC_DisableIRQ(EADC00_IRQn);
    EADC_DISABLE_INT(EADC, BIT0);

    EADC_Close(EADC);

    CLK_DisableModuleClock(EADC_MODULE);

    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_SD0_nCD);
}
