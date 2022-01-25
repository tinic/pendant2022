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
#include "./sdcard.h"
#include "./main.h"
#include "./msc.h"
#include "./stm32wl.h"
#include "./timeline.h"
#include "./version.h"

#include <memory.h>

#include "M480.h"

extern "C" SDH_INFO_T SD0, SD1;

#define TRIM_INIT (SYS_BASE+0x10C)

extern uint8_t volatile g_u8SdInitFlag;
extern int32_t g_TotalSectors;

void SDH0_IRQHandler(void)
{
    unsigned int volatile isr;

    // FMI data abort interrupt
    if (SDH0->GINTSTS & SDH_GINTSTS_DTAIF_Msk)
    {
        /* ResetAllEngine() */
        SDH0->GCTL |= SDH_GCTL_GCTLRST_Msk;
    }

    //----- SD interrupt status
    isr = SDH0->INTSTS;
    if (isr & SDH_INTSTS_BLKDIF_Msk)
    {
        // block down
        SD0.DataReadyFlag = TRUE;
        SDH0->INTSTS = SDH_INTSTS_BLKDIF_Msk;
    }

    if (isr & SDH_INTSTS_CDIF_Msk) // card detect
    {
        //----- SD interrupt status
        // delay 10 us to sync the GPIO and SDH
        {
            uint32_t volatile delay = SystemCoreClock / 1000000 * 10;
            for(; delay > 0UL; delay--)
            {
                __NOP();
            }
            isr = SDH0->INTSTS;
        }

        if (isr & SDH_INTSTS_CDSTS_Msk)
        {
            printf("\nSDH0_IRQHandler card remove !\n");
            SD0.IsCardInsert = FALSE;   // SDISR_CD_Card = 1 means card remove for GPIO mode
            memset(&SD0, 0, sizeof(SDH_INFO_T));
        }
        else
        {
            printf("SDH0_IRQHandler card insert !\n");
            SDH_Open(SDH0, CardDetect_From_GPIO);
            if (SDH_Probe(SDH0))
            {
                g_u8SdInitFlag = 0;
                printf("SDH0_IRQHandler SD initial fail!!\n");
            }
            else
            {
                g_u8SdInitFlag = 1;
                g_TotalSectors = SD0.totalSectorN;
            }
        }
        SDH0->INTSTS = SDH_INTSTS_CDIF_Msk;
    }

    // CRC error interrupt
    if (isr & SDH_INTSTS_CRCIF_Msk)
    {
        if (!(isr & SDH_INTSTS_CRC16_Msk))
        {
            //printf("***** ISR sdioIntHandler(): CRC_16 error !\n");
            // handle CRC error
        }
        else if (!(isr & SDH_INTSTS_CRC7_Msk))
        {
            if (!SD0.R3Flag)
            {
                //printf("***** ISR sdioIntHandler(): CRC_7 error !\n");
                // handle CRC error
            }
        }
        SDH0->INTSTS = SDH_INTSTS_CRCIF_Msk;      // clear interrupt flag
    }

    if (isr & SDH_INTSTS_DITOIF_Msk)
    {
        printf("***** ISR: data in timeout !\n");
        SDH0->INTSTS |= SDH_INTSTS_DITOIF_Msk;
    }

    // Response in timeout interrupt
    if (isr & SDH_INTSTS_RTOIF_Msk)
    {
        printf("***** ISR: response in timeout !\n");
        SDH0->INTSTS |= SDH_INTSTS_RTOIF_Msk;
    }
}

SDCard& SDCard::instance() {
    static SDCard sdcard;
    if (!sdcard.initialized) {
        sdcard.initialized = true;
        sdcard.init();
    }
    return sdcard;
}

void SDCard::process() {
    MSC_ProcessCmd();
}

void SDCard::init() {
    SDH_Open(SDH0, CardDetect_From_GPIO);
    if (SDH_Probe(SDH0) != 0) {
        g_u8SdInitFlag = 0;
        printf("SDCard::init SD initial fail!!\n");
    }
    else {
        g_u8SdInitFlag = 1;
    }

    USBD_Open(&gsInfo, MSC_ClassRequest, NULL);
    USBD_SetConfigCallback(MSC_SetConfig);

    MSC_Init();
    USBD_Start();

    NVIC_SetPriority(USBD_IRQn, 1);
    NVIC_EnableIRQ(USBD_IRQn);
}
