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
#include "M480.h"

#include "./main.h"
#include "./leds.h"
#include "./color.h"
#include "./model.h"

#include <memory.h>

#define SPI1_MASTER_TX_DMA_CH   0
#define SPI2_MASTER_TX_DMA_CH   1

Leds &Leds::instance() {
    static Leds leds;
    if (!leds.initialized) {
        leds.initialized = true;
        leds.init();
    }
    return leds;
}

struct Leds::Map Leds::map;

const Leds::lut_table Leds::lut;

void Leds::init() {
    // LED_ON
    GPIO_SetMode(PC, BIT7, GPIO_MODE_OUTPUT);
    // Turn Mosfet on
    PC7 = 0;

    half();

    SPI_Open(SPI1, SPI_MASTER, SPI_MODE_0, 8, 4000000);
    SPI_Open(SPI2, SPI_MASTER, SPI_MODE_0, 8, 4000000);

    GPIO_SetMode(PC, BIT6, GPIO_MODE_OUTPUT);
    PC6 = 0;

    GPIO_SetMode(PA, BIT15, GPIO_MODE_OUTPUT);
    PA15 = 0;

#ifdef USE_SPI_DMA
    PDMA_Open(PDMA, (1UL << SPI1_MASTER_TX_DMA_CH) | (1UL << SPI2_MASTER_TX_DMA_CH));

    PDMA_SetTransferCnt(PDMA, SPI1_MASTER_TX_DMA_CH, PDMA_WIDTH_8, ledsDMABuf[0].size() * sizeof(uint32_t));
    PDMA_SetTransferAddr(PDMA, SPI1_MASTER_TX_DMA_CH, reinterpret_cast<uintptr_t>(ledsDMABuf[0].data()), PDMA_SAR_INC, reinterpret_cast<uintptr_t>(&SPI1->TX), PDMA_DAR_FIX);
    PDMA_SetTransferMode(PDMA, SPI1_MASTER_TX_DMA_CH, PDMA_SPI1_TX, FALSE, 0);
    PDMA_SetBurstType(PDMA, SPI1_MASTER_TX_DMA_CH, PDMA_REQ_SINGLE, 0);
    PDMA->DSCT[SPI1_MASTER_TX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;

    PDMA_SetTransferCnt(PDMA, SPI2_MASTER_TX_DMA_CH, PDMA_WIDTH_8, ledsDMABuf[1].size() * sizeof(uint32_t));
    PDMA_SetTransferAddr(PDMA, SPI2_MASTER_TX_DMA_CH, reinterpret_cast<uintptr_t>(ledsDMABuf[1].data()), PDMA_SAR_INC, reinterpret_cast<uintptr_t>(&SPI2->TX), PDMA_DAR_FIX);
    PDMA_SetTransferMode(PDMA, SPI2_MASTER_TX_DMA_CH, PDMA_SPI2_TX, FALSE, 0);
    PDMA_SetBurstType(PDMA, SPI2_MASTER_TX_DMA_CH, PDMA_REQ_SINGLE, 0);
    PDMA->DSCT[SPI2_MASTER_TX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;
#endif  // #ifdef USE_SPI_DMA

    printf("Leds initialized.\n");
}

void Leds::prepare() {
    static color::convert converter;

    float brightness = Model::instance().Brightness();

    uint32_t *ptr0 = ledsDMABuf[0].data();
    uint32_t *ptr1 = ledsDMABuf[1].data();
    
    for (size_t c = 0; c < circleLedsN; c++) {
        color::rgba<uint16_t> pixel0(color::rgba<uint16_t>(converter.CIELUV2sRGB(circleLeds[0][c]*brightness)).fix_for_ws2816());
        color::rgba<uint16_t> pixel1(color::rgba<uint16_t>(converter.CIELUV2sRGB(circleLeds[1][(c-1)%circleLedsN]*brightness)).fix_for_ws2816());

        auto convert_to_one_wire_spi = [] (uint32_t *p, uint16_t v) {
            *p++ = lut[(v>>8)&0xFF];
            *p++ = lut[(v>>0)&0xFF];
            return p;
        };

        ptr0 = convert_to_one_wire_spi(ptr0, pixel0.g);
        ptr0 = convert_to_one_wire_spi(ptr0, pixel0.r);
        ptr0 = convert_to_one_wire_spi(ptr0, pixel0.b);
        ptr1 = convert_to_one_wire_spi(ptr1, pixel1.g);
        ptr1 = convert_to_one_wire_spi(ptr1, pixel1.r);
        ptr1 = convert_to_one_wire_spi(ptr1, pixel1.b);

    }

    for (size_t c = 0; c < birdLedsN; c++) {
        color::rgba<uint16_t> pixel0(color::rgba<uint16_t>(converter.CIELUV2sRGB(birdLeds[0][c]*brightness)).fix_for_ws2816());
        color::rgba<uint16_t> pixel1(color::rgba<uint16_t>(converter.CIELUV2sRGB(birdLeds[1][c]*brightness)).fix_for_ws2816());

        auto convert_to_one_wire_spi = [] (uint32_t *p, uint16_t v) {
            *p++ = lut[(v>>8)&0xFF];
            *p++ = lut[(v>>0)&0xFF];
            return p;
        };

        ptr0 = convert_to_one_wire_spi(ptr0, pixel0.g);
        ptr0 = convert_to_one_wire_spi(ptr0, pixel0.r);
        ptr0 = convert_to_one_wire_spi(ptr0, pixel0.b);
        ptr1 = convert_to_one_wire_spi(ptr1, pixel1.g);
        ptr1 = convert_to_one_wire_spi(ptr1, pixel1.r);
        ptr1 = convert_to_one_wire_spi(ptr1, pixel1.b);
    }

}

__attribute__ ((hot, optimize("Os"), flatten))
void Leds::transfer() {
    prepare();

#ifdef USE_SPI_DMA

    PDMA_SetTransferCnt(PDMA, SPI1_MASTER_TX_DMA_CH, PDMA_WIDTH_8, ledsDMABuf[0].size() * sizeof(uint32_t));
    PDMA_SetTransferMode(PDMA, SPI1_MASTER_TX_DMA_CH, PDMA_SPI1_TX, FALSE, 0);
    SPI_TRIGGER_TX_PDMA(SPI1);

    PDMA_SetTransferCnt(PDMA, SPI2_MASTER_TX_DMA_CH, PDMA_WIDTH_8, ledsDMABuf[1].size() * sizeof(uint32_t));
    PDMA_SetTransferMode(PDMA, SPI2_MASTER_TX_DMA_CH, PDMA_SPI2_TX, FALSE, 0);
    SPI_TRIGGER_TX_PDMA(SPI2);

#else  // #ifdef USE_DMA

    for(size_t c = 0; c < ledsDMABuf[0].size() * sizeof(uint32_t); c++) {
        while(SPI_GET_TX_FIFO_FULL_FLAG(SPI1) == 1) {}
        SPI_WRITE_TX(SPI1, reinterpret_cast<uint8_t *>(ledsDMABuf[0].data())[c]);
    }

    for(size_t c = 0; c < ledsDMABuf[1].size() * sizeof(uint32_t); c++) {
        while(SPI_GET_TX_FIFO_FULL_FLAG(SPI2) == 1) {}
        SPI_WRITE_TX(SPI2, reinterpret_cast<uint8_t *>(ledsDMABuf[1].data())[c]);
    }

#endif  // #ifdef USE_DMA

}
