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
#include "./timeline.h"
#include "./version.h"

#include <memory.h>

#include "M480.h"

/*----------------------------------------------------------------------------*/
/*!<USB Device Descriptor */
static uint8_t gu8DeviceDescriptor[] __attribute__((aligned(4))) =
{
    LEN_DEVICE,                                         /* bLength */
    DESC_DEVICE,                                        /* bDescriptorType */
    0x10, 0x01,                                         /* bcdUSB */
    0x00,                                               /* bDeviceClass */
    0x00,                                               /* bDeviceSubClass */
    0x00,                                               /* bDeviceProtocol */
    EP0_MAX_PKT_SIZE,                                   /* bMaxPacketSize0 */
    /* idVendor */
    USBD_VID & 0x00FF,
    ((USBD_VID & 0xFF00) >> 8),
    /* idProduct */
    USBD_PID & 0x00FF,
    ((USBD_PID & 0xFF00) >> 8),
    0x00, 0x00,                                         /* bcdDevice */
    0x01,                                               /* iManufacture */
    0x02,                                               /* iProduct */
    0x03,                                               /* iSerialNumber -  is required for BOT device */
    0x01                                                /* bNumConfigurations */
};

/*!<USB Configure Descriptor */
static uint8_t gu8ConfigDescriptor[] __attribute__((aligned(4))) =
{
    LEN_CONFIG,                                         // bLength
    DESC_CONFIG,                                        // bDescriptorType
    (LEN_CONFIG+LEN_INTERFACE+LEN_ENDPOINT*2), 0x00,    // wTotalLength
    0x01,                                               // bNumInterfaces
    0x01,                                               // bConfigurationValue
    0x00,                                               // iConfiguration
    0xC0,                                               // bmAttributes
    0x32,                                               // MaxPower

    /* const BYTE cbyInterfaceDescriptor[LEN_INTERFACE] = */
    LEN_INTERFACE,                                      // bLength
    DESC_INTERFACE,                                     // bDescriptorType
    0x00,                                               // bInterfaceNumber
    0x00,                                               // bAlternateSetting
    0x02,                                               // bNumEndpoints
    0x08,                                               // bInterfaceClass
    0x05,                                               // bInterfaceSubClass
    0x50,                                               // bInterfaceProtocol
    0x00,                                               // iInterface

    /* const BYTE cbyEndpointDescriptor1[LEN_ENDPOINT] = */
    LEN_ENDPOINT,                                       // bLength
    DESC_ENDPOINT,                                      // bDescriptorType
    (BULK_IN_EP_NUM | EP_INPUT),                        // bEndpointAddress
    EP_BULK,                                            // bmAttributes
    EP2_MAX_PKT_SIZE, 0x00,                             // wMaxPacketSize
    0x00,                                               // bInterval

    /* const BYTE cbyEndpointDescriptor2[LEN_ENDPOINT] = */
    LEN_ENDPOINT,                                       // bLength
    DESC_ENDPOINT,                                      // bDescriptorType
    BULK_OUT_EP_NUM,                                    // bEndpointAddress
    EP_BULK,                                            // bmAttributes
    EP3_MAX_PKT_SIZE, 0x00,                             // wMaxPacketSize
    0x00                                                // bInterval
};

/*!<USB Language String Descriptor */
static uint8_t gu8StringLang[4] __attribute__((aligned(4))) =
{
    4,              /* bLength */
    DESC_STRING,    /* bDescriptorType */
    0x09, 0x04
};

/*!<USB Vendor String Descriptor */
static uint8_t gu8VendorStringDesc[] __attribute__((aligned(4))) =
{
    16,
    DESC_STRING,
    'N', 0, 'u', 0, 'v', 0, 'o', 0, 't', 0, 'o', 0, 'n', 0
};

/*!<USB Product String Descriptor */
static uint8_t gu8ProductStringDesc[] __attribute__((aligned(4))) =
{
    22,             /* bLength          */
    DESC_STRING,    /* bDescriptorType  */
    'U', 0, 'S', 0, 'B', 0, ' ', 0, 'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0
};

static uint8_t gu8StringSerial[] __attribute__((aligned(4))) =
{
    26,             // bLength
    DESC_STRING,    // bDescriptorType
    'A', 0, '0', 0, '0', 0, '0', 0, '0', 0, '8', 0, '0', 0, '4', 0, '0', 0, '1', 0, '1', 0, '5', 0

};

static const uint8_t *gpu8UsbString[4] =
{
    gu8StringLang,
    gu8VendorStringDesc,
    gu8ProductStringDesc,
    gu8StringSerial
};

const S_USBD_INFO_T gsInfo =
{
    (uint8_t *)gu8DeviceDescriptor,
    (uint8_t *)gu8ConfigDescriptor,
    (uint8_t **)gpu8UsbString,
    NULL,
    NULL,
    NULL,
    NULL
};

enum {
    markerBootable = 0xB007AB1E,

    markerTesting = 0x7E577E57,
    markerRelease = 0x5ADD1ED0,

    markerBootloader = 0xB00710AD,
    markerBootloaded = 0x1ED54B0B,
};

static constexpr uint32_t swap(uint32_t v) {
    return ((v >> 24) & 0xFF) | ((v & 0xFF) << 24) | ((v >> 8) & 0xFF00) | ((v & 0xFF00) << 8);
}

extern "C" const uint32_t __attribute__((used, section(".metadata"))) metadata[] = {
    swap(markerBootable),
#if defined(TESTING)
    swap(markerTesting),
#else
    swap(markerRelease),
#endif // defined(TESTING)
#if defined(BOOTLOADER)
    swap(markerBootloader),
#else // #if defined(BOOTLOADER)
    swap(markerBootloaded),
#endif // #if defined(BOOTLOADER)
    GIT_REV_COUNT_INT,
    0, 0, 0, 0
};

extern "C" SDH_INFO_T SD0, SD1;

#define TRIM_INIT (SYS_BASE+0x10C)

extern uint8_t volatile g_u8SdInitFlag;
extern uint32_t g_TotalSectors;

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

extern "C" DWORD get_fattime(void) {
    return 0; // TODO
}

static constexpr uint8_t DEV_MMC = 0;

DSTATUS disk_status(BYTE pdrv) {
    if (pdrv != 0) {
        return STA_NOINIT;
    }

    return 0;
}

DSTATUS disk_initialize(BYTE pdrv) {
    if (pdrv != 0) {
        return STA_NOINIT;
    }

    return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv != 0) {
        return RES_PARERR;
    }
    MSC_ReadMedia(sector, count, buff);
    return RES_OK;
}

#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv != 0) {
        return RES_PARERR;
    }
    MSC_WriteMedia(sector, count, buff);
    return RES_OK;
}
#endif

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
    if (pdrv != 0) {
        return RES_PARERR;
    }
    switch (cmd) {
    case CTRL_SYNC: {
        printf("CTRL_SYNC\n");
        return RES_OK;
    } break;
    }
    return RES_PARERR;
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

bool SDCard::readFromDataFile(uint8_t* outBuf, size_t offset, size_t size) {
    if (!datafile_present) {
        return false;
    }

    FIL Fil;
    if (f_open(&Fil, "data.bin", FA_READ | FA_OPEN_EXISTING) == FR_OK) {
        f_lseek(&Fil, offset);
        UINT readLen = size;
        f_read(&Fil, outBuf, readLen, &readLen);
        f_close(&Fil);
        return readLen == size;
    }

    return false;
}

void SDCard::findDataFile() {
    if (!mounted) {
        return;
    }

    FIL Fil;
    if (f_open(&Fil, "data.bin", FA_READ | FA_OPEN_EXISTING) == FR_OK) {
        printf("SDCard: Found data.bin!\n");
        datafile_present = true;
        f_close(&Fil);
    }
}

void SDCard::findFirmware() {
    if (!mounted) {
        return;
    }

    const char* filename = "firmware.bin";

    FILINFO FilInfo = {};
    f_stat(filename, &FilInfo);
    if (FilInfo.fsize < 32768) {
        return;
    }

    FIL Fil = {};
    if (f_open(&Fil, filename, FA_READ | FA_OPEN_EXISTING) != FR_OK) {
        return;
    }

    printf("SDCard: Found firmware.bin!\n");

    const size_t markerSize = 32;

    f_lseek(&Fil, FilInfo.fsize - markerSize);

    BYTE buffer[markerSize];
    UINT readLen = markerSize;
    f_read(&Fil, buffer, readLen, &readLen);
    if (readLen != markerSize) {
        return;
    }

    auto matchMarker = [=](uint32_t pos, uint32_t marker) {
        // Big-endian
        return buffer[pos * sizeof(uint32_t) + 0] == ((marker >> 24) & 0xFF) && buffer[pos * sizeof(uint32_t) + 1] == ((marker >> 16) & 0xFF) && buffer[pos * sizeof(uint32_t) + 2] == ((marker >> 8) & 0xFF) && buffer[pos * sizeof(uint32_t) + 3] == ((marker >> 0) & 0xFF);
    };

    auto readUint32 = [=](uint32_t pos) {
        return (uint32_t(buffer[pos * sizeof(uint32_t) + 0]) << 0) | (uint32_t(buffer[pos * sizeof(uint32_t) + 1]) << 8) | (uint32_t(buffer[pos * sizeof(uint32_t) + 2]) << 16) | (uint32_t(buffer[pos * sizeof(uint32_t) + 3]) << 24);
    };

    if (!matchMarker(0, markerBootable)) {
        return;
    }

    printf("SDCard: Found valid firmware sentinel!\n");
    if (!matchMarker(1, markerTesting) && !matchMarker(1, markerRelease)) {
        return;
    }

    firmware_release = matchMarker(1, markerRelease);
    printf("SDCard: Firmware type %d\n", firmware_release ? 1 : 0);
    if (!matchMarker(2, markerBootloader) && !matchMarker(2, markerBootloaded)) {
        return;
    }

    firmware_bootloaded = matchMarker(2, markerBootloaded);
    printf("SDCard: Firmware variant %d\n", firmware_bootloaded ? 1 : 0);

    firmware_revision = readUint32(3);
    printf("SDCard: Firmware rev %d\n", int(firmware_revision));

    f_close(&Fil);
}

bool SDCard::inserted() const {
    return g_u8SdInitFlag;
}

uint32_t SDCard::blocks() const {
    return g_TotalSectors;
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

    if (f_mount(&FatFs, "", 0) == FR_OK) {
        printf("SDCard: mounted!\n");
        mounted = true;
        findFirmware();
#ifndef BOOTLOADER
        findDataFile();
#endif // #ifndef BOOTLOADER
    }
}
