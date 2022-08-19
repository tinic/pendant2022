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
#include "./main.h"

#include "M480.h"
#include "./version.h"

#include <stdio.h>

void delay_us(int usec) {
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

#ifdef BOOTLOADER
extern void bootloader_entry(void);
#else  // #ifdef BOOTLOADER
extern void pendant_entry(void);
#endif  // #ifdef BOOTLOADER

static void SYS_Init(void)
{
    SYS_UnlockReg();

    // Set up pins

    // I2C1
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA7MFP_Msk | SYS_GPA_MFPL_PA6MFP_Msk);
    SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA7MFP_I2C1_SCL | SYS_GPA_MFPL_PA6MFP_I2C1_SDA);

    // I2C2
    SYS->GPA_MFPH &= ~(SYS_GPA_MFPH_PA11MFP_Msk | SYS_GPA_MFPH_PA10MFP_Msk);
    SYS->GPA_MFPH |= (SYS_GPA_MFPH_PA11MFP_I2C2_SCL | SYS_GPA_MFPH_PA10MFP_I2C2_SDA);

    // I2S0
    SYS->GPA_MFPH &= ~(SYS_GPA_MFPH_PA14MFP_Msk | SYS_GPA_MFPH_PA13MFP_Msk | SYS_GPA_MFPH_PA12MFP_Msk);
    SYS->GPA_MFPH |= (SYS_GPA_MFPH_PA14MFP_I2S0_DI | SYS_GPA_MFPH_PA13MFP_I2S0_MCLK | SYS_GPA_MFPH_PA12MFP_I2S0_BCLK);

    // ICE
    SYS->GPF_MFPL &= ~(SYS_GPF_MFPL_PF1MFP_Msk | SYS_GPF_MFPL_PF0MFP_Msk);
    SYS->GPF_MFPL |= (SYS_GPF_MFPL_PF1MFP_ICE_CLK | SYS_GPF_MFPL_PF0MFP_ICE_DAT);

    // GPIO
    SYS->GPA_MFPH &= ~(SYS_GPA_MFPH_PA9MFP_Msk | SYS_GPA_MFPH_PA8MFP_Msk);
    SYS->GPA_MFPH |= (SYS_GPA_MFPH_PA9MFP_GPIO | SYS_GPA_MFPH_PA8MFP_GPIO);
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA3MFP_Msk | SYS_GPA_MFPL_PA4MFP_Msk);
    SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA3MFP_GPIO | SYS_GPA_MFPL_PA4MFP_GPIO);

    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB15MFP_Msk | SYS_GPB_MFPH_PB10MFP_Msk | SYS_GPB_MFPH_PB9MFP_Msk | SYS_GPB_MFPH_PB8MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB15MFP_GPIO | SYS_GPB_MFPH_PB10MFP_GPIO | SYS_GPB_MFPH_PB9MFP_GPIO | SYS_GPB_MFPH_PB8MFP_GPIO);
    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB7MFP_Msk);
    SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB7MFP_GPIO);

    SYS->GPC_MFPH &= ~(SYS_GPC_MFPH_PC14MFP_Msk);
    SYS->GPC_MFPH |= (SYS_GPC_MFPH_PC14MFP_GPIO);
    SYS->GPC_MFPL &= ~(SYS_GPC_MFPL_PC7MFP_Msk | SYS_GPC_MFPL_PC1MFP_Msk | SYS_GPC_MFPL_PC0MFP_Msk);
    SYS->GPC_MFPL |= (SYS_GPC_MFPL_PC7MFP_GPIO | SYS_GPC_MFPL_PC1MFP_GPIO | SYS_GPC_MFPL_PC0MFP_GPIO);

    // SD0
//    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk);
//    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_SD0_nCD);
//    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB5MFP_Msk | SYS_GPB_MFPL_PB4MFP_Msk | SYS_GPB_MFPL_PB3MFP_Msk | SYS_GPB_MFPL_PB2MFP_Msk | SYS_GPB_MFPL_PB1MFP_Msk | SYS_GPB_MFPL_PB0MFP_Msk);
//    SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB5MFP_SD0_DAT3 | SYS_GPB_MFPL_PB4MFP_SD0_DAT2 | SYS_GPB_MFPL_PB3MFP_SD0_DAT1 | SYS_GPB_MFPL_PB2MFP_SD0_DAT0 | SYS_GPB_MFPL_PB1MFP_SD0_CLK | SYS_GPB_MFPL_PB0MFP_SD0_CMD);

    // SPI0
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA2MFP_Msk | SYS_GPA_MFPL_PA1MFP_Msk | SYS_GPA_MFPL_PA0MFP_Msk);
    SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA2MFP_SPI0_CLK | SYS_GPA_MFPL_PA1MFP_SPI0_MISO | SYS_GPA_MFPL_PA0MFP_SPI0_MOSI);

    // SPI1_MOSI
    SYS->GPC_MFPL &= ~(SYS_GPC_MFPL_PC6MFP_Msk);
    SYS->GPC_MFPL |= (SYS_GPC_MFPL_PC6MFP_SPI1_MOSI);

    // SPI2_MOSI
    SYS->GPA_MFPH &= ~(SYS_GPA_MFPH_PA15MFP_Msk);
    SYS->GPA_MFPH |= (SYS_GPA_MFPH_PA15MFP_SPI2_MOSI);

    // UART5_TX
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA5MFP_Msk);
    SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA5MFP_UART5_TXD);

    // X32
    SYS->GPF_MFPL &= ~(SYS_GPF_MFPL_PF5MFP_Msk | SYS_GPF_MFPL_PF4MFP_Msk);
    SYS->GPF_MFPL |= (SYS_GPF_MFPL_PF5MFP_X32_IN | SYS_GPF_MFPL_PF4MFP_X32_OUT);

    // XT1
    SYS->GPF_MFPL &= ~(SYS_GPF_MFPL_PF3MFP_Msk | SYS_GPF_MFPL_PF2MFP_Msk);
    SYS->GPF_MFPL |= (SYS_GPF_MFPL_PF3MFP_XT1_IN | SYS_GPF_MFPL_PF2MFP_XT1_OUT);

    // Init clocks
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);

    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);

    CLK_DisablePLL();

    CLK_SetCoreClock(96000000UL);

    CLK->PCLKDIV = (CLK_PCLKDIV_PCLK0DIV8 | CLK_PCLKDIV_PCLK1DIV8);

    CLK_EnableModuleClock(FMCIDLE_MODULE); // HCLK 96Mhz
    CLK_EnableModuleClock(I2C1_MODULE); // PCLK1 12Mhz
    CLK_EnableModuleClock(I2C2_MODULE); // PCLK0 12Mhz
    CLK_EnableModuleClock(ISP_MODULE); // HIRC 12Mhz
    CLK_EnableModuleClock(PDMA_MODULE); // HCLK 96Mhz

    // SDH0
//    CLK_EnableModuleClock(SDH0_MODULE);
//    CLK_SetModuleClock(SDH0_MODULE, CLK_CLKSEL0_SDH0SEL_HCLK, CLK_CLKDIV0_SDH0(1)); // 96Mhz

    // SPI0
    CLK_EnableModuleClock(SPI0_MODULE);
    CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_HIRC, MODULE_NoMsk); // HIRC 12Mhz

    // SPI1_MOSI
    CLK_EnableModuleClock(SPI1_MODULE);
    CLK_SetModuleClock(SPI1_MODULE, CLK_CLKSEL2_SPI1SEL_HIRC, MODULE_NoMsk); // HIRC 12Mhz

    // SPI2_MOSI
    CLK_EnableModuleClock(SPI2_MODULE);
    CLK_SetModuleClock(SPI2_MODULE, CLK_CLKSEL2_SPI2SEL_HIRC, MODULE_NoMsk); // HIRC 12Mhz

    // TMR0
    CLK_EnableModuleClock(TMR0_MODULE);
    CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_LIRC, MODULE_NoMsk); // LIRC 10Khz

    // TMR1
    CLK_EnableModuleClock(TMR1_MODULE);
    CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_LIRC, MODULE_NoMsk); // LIRC 10Khz

    // UART5_TX
    CLK_EnableModuleClock(UART5_MODULE);
    CLK_SetModuleClock(UART5_MODULE, CLK_CLKSEL3_UART5SEL_HIRC, CLK_CLKDIV4_UART5(1)); // HIRC 12Mhz

    SystemCoreClockUpdate();

    SYS_LockReg();
}

static void SYS_DeInit(void)
{
    CLK_DisableModuleClock(FMCIDLE_MODULE);
    CLK_DisableModuleClock(I2C1_MODULE);
    CLK_DisableModuleClock(I2C2_MODULE);
    CLK_DisableModuleClock(ISP_MODULE);
    CLK_DisableModuleClock(PDMA_MODULE);
    CLK_DisableModuleClock(SDH0_MODULE);
    CLK_DisableModuleClock(SPI0_MODULE);
    CLK_DisableModuleClock(SPI1_MODULE);
    CLK_DisableModuleClock(SPI2_MODULE);
    CLK_DisableModuleClock(TMR0_MODULE);
    CLK_DisableModuleClock(TMR1_MODULE);
    CLK_DisableModuleClock(UART5_MODULE);
}

int main(void)
{
    SYS_Init();

    UART_Open(UART5, 115200);

    SYS_UnlockReg();
    FMC_Open();

    printf("================================================================================\n");
    printf("Pendant 2021 "
#if defined(TESTING)
#if defined(BOOTLOADER)
        "Bootloader Testing"
#else  // #if defined(BOOTLOADER)
        "Main Testing"
#endif  // #if defined(BOOTLOADER)
#elif defined(BOOTLOADER)
        "Bootloader"
#else  // #if defined(BOOTLOADER)
        "Main"
#endif  // #if defined(BOOTLOADER)
        "\nPID(0x%08x) UID(0x%08x%08x%08x)\n"
        "Build %s\n",
        (unsigned int)SYS_ReadPDID(),
        (unsigned int)FMC_ReadUID(0),
        (unsigned int)FMC_ReadUID(1),
        (unsigned int)FMC_ReadUID(2),
        "r" GIT_REV_COUNT " (" GIT_SHORT_SHA ") " GIT_COMMIT_DATE);
    printf("--------------------------------------------------------------------------------\n");

    printf("SystemCoreClock (%d Hz) HXT (%d Hz)\n", (int)(SystemCoreClock), (int)__HXT);

    FMC_Close();
    SYS_LockReg();

#if defined(BOOTLOADER)

    FMC_Open();

    if (PF2 != 0) {        
        SYS_DeInit();
        
        FMC_SetVectorPageAddr(FIRMWARE_ADDR + FIRMWARE_START);

        SYS_ResetChip();

        for (;;) { }
    }

    bootloader_entry();

#else  // #if defined(BOOTLOADER)

    pendant_entry();

#endif  // #if defined(BOOTLOADER)

    for (;;) {
    }

    SYS_DeInit();

    return 0;
}
