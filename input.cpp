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
#include "./input.h"
#include "./timeline.h"
#include "./model.h"

#include "M480.h"

extern "C" {

void GPA_IRQHandler(void)
{
    // ACCEL_INT1
    if(GPIO_GET_INT_FLAG(PA, BIT9))
    {
        GPIO_CLR_INT_FLAG(PA, BIT8);
    }

    // ACCEL_INT2
    if(GPIO_GET_INT_FLAG(PA, BIT8))
    {
        GPIO_CLR_INT_FLAG(PA, BIT8);
    }
}

void GPB_IRQHandler(void)
{
    // SD0_NCD
    if(GPIO_GET_INT_FLAG(PB, BIT12))
    {
        GPIO_CLR_INT_FLAG(PB, BIT12);
    }

    // DSEL
    if(GPIO_GET_INT_FLAG(PB, BIT15))
    {
        GPIO_CLR_INT_FLAG(PB, BIT15);
    }

    // SW1
    if(GPIO_GET_INT_FLAG(PB, BIT9))
    {
        Timeline::instance().TopDisplay().ProcessSwitch1(PB9 ? true : false);
        if (PB9) Model::instance().IncSwitch1Count();
        GPIO_CLR_INT_FLAG(PB, BIT9);
    }

    // SW2
    if(GPIO_GET_INT_FLAG(PB, BIT8))
    {
        Timeline::instance().TopDisplay().ProcessSwitch2(PB8 ? true : false);
        if (PB8) Model::instance().IncSwitch2Count();
        GPIO_CLR_INT_FLAG(PB, BIT8);
    }

    // SW3
    if(GPIO_GET_INT_FLAG(PB, BIT7))
    {
        Timeline::instance().TopDisplay().ProcessSwitch3(PB7 ? true : false);
        if (PB7) Model::instance().IncSwitch2Count();
        GPIO_CLR_INT_FLAG(PB, BIT7);
    }
}

void GPC_IRQHandler(void)
{
    // RF_DIO1
    if(GPIO_GET_INT_FLAG(PC, BIT0))
    {
        GPIO_CLR_INT_FLAG(PC, BIT0);
    }

    // BQ_INT
    if(GPIO_GET_INT_FLAG(PC, BIT14))
    {
        GPIO_CLR_INT_FLAG(PC, BIT14);
    }
}

}

Input &Input::instance() {
    static Input input;
    if (!input.initialized) {
        input.initialized = true;
        input.init();
        printf("Input initialized.\n");
    }
    return input;
}

void Input::init() {
    GPIO_SET_DEBOUNCE_TIME(GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_256);

    // ACCEL_INT1
    GPIO_SetMode(PA, BIT9, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PA, BIT9, GPIO_PUSEL_PULL_UP);
    GPIO_EnableInt(PA, 9, GPIO_INT_BOTH_EDGE);

    // ACCEL_INT2
    GPIO_SetMode(PA, BIT8, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PA, BIT8, GPIO_PUSEL_PULL_UP);
    GPIO_EnableInt(PA, 8, GPIO_INT_BOTH_EDGE);

    // SD0_NCD
    GPIO_SetMode(PB, BIT12, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PB, BIT12, GPIO_PUSEL_PULL_UP);
    GPIO_EnableInt(PB, 12, GPIO_INT_BOTH_EDGE);

    // DSEL
    GPIO_SetMode(PB, BIT15, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PB, BIT15, GPIO_PUSEL_PULL_UP);
    GPIO_EnableInt(PB, 15, GPIO_INT_BOTH_EDGE);

    // SW1
    GPIO_SetMode(PB, BIT9, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PB, BIT9, GPIO_PUSEL_PULL_UP);
    GPIO_ENABLE_DEBOUNCE(PB, BIT9);
    GPIO_CLR_INT_FLAG(PB, BIT9);
    GPIO_EnableInt(PB, 9, GPIO_INT_BOTH_EDGE);

    // SW2
    GPIO_SetMode(PB, BIT8, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PB, BIT8, GPIO_PUSEL_PULL_UP);
    GPIO_ENABLE_DEBOUNCE(PB, BIT8);
    GPIO_CLR_INT_FLAG(PB, BIT8);
    GPIO_EnableInt(PB, 8, GPIO_INT_BOTH_EDGE);

    // SW3
    GPIO_SetMode(PB, BIT7, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PB, BIT7, GPIO_PUSEL_PULL_UP);
    GPIO_ENABLE_DEBOUNCE(PB, BIT7);
    GPIO_CLR_INT_FLAG(PB, BIT7);
    GPIO_EnableInt(PB, 7, GPIO_INT_BOTH_EDGE);

    // RF_DIO1
    GPIO_SetMode(PC, BIT0, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PC, BIT0, GPIO_PUSEL_PULL_UP);
    GPIO_EnableInt(PC, 0, GPIO_INT_BOTH_EDGE);

    // BQ_INT
    GPIO_SetMode(PC, BIT14, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PC, BIT14, GPIO_PUSEL_PULL_UP);
    GPIO_EnableInt(PC, 14, GPIO_INT_BOTH_EDGE);

    NVIC_EnableIRQ(GPA_IRQn);
    NVIC_SetPriority(GPA_IRQn, 5);

    NVIC_EnableIRQ(GPB_IRQn);
    NVIC_SetPriority(GPB_IRQn, 5);

    NVIC_EnableIRQ(GPC_IRQn);
    NVIC_SetPriority(GPC_IRQn, 5);

}
