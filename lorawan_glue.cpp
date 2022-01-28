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
#include "./lorawan.h"
#include "./main.h"

#include "version.h"
#include "timeline.h"

// Nuvonton
#include "M480.h"

// LoraWAN-node
#include "system/gpio.h"
#include "system/timer.h"
#include "system/nvmm.h"
#include "board.h"
#include "sx126x-board.h"

extern "C" {

void RtcDelayMs(uint32_t delay);

void DelayMsMcu(uint32_t ms) {
    RtcDelayMs(ms);
}

void BoardCriticalSectionBegin(uint32_t *mask) {
    __disable_irq();
}

void BoardCriticalSectionEnd(uint32_t *mask) {
    __enable_irq();
}

void BoardResetMcu(void) {
    CRITICAL_SECTION_BEGIN();
    NVIC_SystemReset();
    CRITICAL_SECTION_END();
}

void BoardGetUniqueId(uint8_t *id) {
    uint32_t ID[4];

    ID[0] = SYS_ReadPDID();
    ID[1] = FMC_ReadUID(0);
    ID[2] = FMC_ReadUID(1);
    ID[3] = FMC_ReadUID(2);

    id[7] = ((ID[0]) + (ID[3])) >> 24;
    id[6] = ((ID[0]) + (ID[3])) >> 16;
    id[5] = ((ID[0]) + (ID[3])) >>  8;
    id[4] = ((ID[0]) + (ID[3])) >>  0;
    id[3] = ((ID[1]) + (ID[4])) >> 24;
    id[2] = ((ID[1]) + (ID[4])) >> 16;
    id[1] = ((ID[1]) + (ID[4])) >>  8;
    id[0] = ((ID[1]) + (ID[4])) >>  0;
}

uint8_t BoardGetBatteryLevel(void) {
    return 0;
}

static RadioOperatingModes_t OperatingMode = MODE_SLEEP;

void SX126xIoInit(void) {

    // RF_NSS
    GPIO_SetMode(PA, BIT3, GPIO_MODE_OUTPUT);
    GPIO_SetPullCtl(PA, BIT3, GPIO_PUSEL_DISABLE);
    PA3 = 0;
    // RF_NRESET
    GPIO_SetMode(PA, BIT4, GPIO_MODE_OUTPUT);
    GPIO_SetPullCtl(PA, BIT4, GPIO_PUSEL_DISABLE);
    PA4 = 1;
    // RF_DIO1
    GPIO_SetMode(PC, BIT0, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PC, BIT0, GPIO_PUSEL_DISABLE);
    GPIO_EnableInt(PC, 0, GPIO_INT_RISING);
    PC0 = 0;
    // RF_BUSY
    GPIO_SetMode(PC, BIT1, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PC, BIT1, GPIO_PUSEL_DISABLE);
    PC1 = 0;

    SPI_Open(SPI0, SPI_MASTER, SPI_MODE_0, 8, 10000000);
}

void SX126xIoIrqInit(DioIrqHandler dioIrq) {
    // NOP
}

void SX126xIoDeInit(void) {
    // NOP
}

void SX126xIoDbgInit(void) {
    // NOP
}

void SX126xIoTcxoInit(void) {
    // NOP
}

uint32_t SX126xGetBoardTcxoWakeupTime(void) {
    return 0;
}

void SX126xIoRfSwitchInit(void) {
    SX126xSetDio2AsRfSwitchCtrl(true);
}

RadioOperatingModes_t SX126xGetOperatingMode(void) {
    return OperatingMode;
}

void SX126xSetOperatingMode(RadioOperatingModes_t mode) {
    OperatingMode = mode;
}

void SX126xReset(void) {
    DelayMsMcu(10);
    PA4 = 0;
    DelayMsMcu(20);
    PA4 = 1;
    DelayMsMcu(10);
}

void SX126xWaitOnBusy(void) {
    // RF_BUSY
    while(PC1 == 1) {}
}

void SX126xWakeup(void) {
    CRITICAL_SECTION_BEGIN();

    // RF_NSS
    PA3 = 0;

    while(SPI_GET_TX_FIFO_FULL_FLAG(SPI0) == 1) {}
    SPI_WRITE_TX(SPI0, RADIO_GET_STATUS);
    while(SPI_GET_TX_FIFO_FULL_FLAG(SPI0) == 1) {}
    SPI_WRITE_TX(SPI0, 0x00);
    while(SPI_GET_TX_FIFO_EMPTY_FLAG(SPI0) == 0) {}

    // RF_NSS
    PA3 = 1;

    SX126xWaitOnBusy();

    SX126xSetOperatingMode(MODE_STDBY_RC);

    CRITICAL_SECTION_END();
}

void SX126xWriteCommand(RadioCommands_t command, uint8_t *buffer, uint16_t size) {
    SX126xCheckDeviceReady();

    // RF_NSS
    PA3 = 0;

    SPI_WRITE_TX(SPI0, (uint8_t)command);
    while(SPI_IS_BUSY(SPI0));
    for(size_t i = 0; i < (size_t)size; i++) {
        SPI_WRITE_TX(SPI0, buffer[i]);
        while(SPI_IS_BUSY(SPI0));
    }

    // RF_NSS
    PA3 = 1;

    if(command != RADIO_SET_SLEEP)
    {
        SX126xWaitOnBusy();
    }
}

uint8_t SX126xReadCommand(RadioCommands_t command, uint8_t *buffer, uint16_t size) {
    uint8_t status = 0;

    SX126xCheckDeviceReady();

    // RF_NSS
    PA3 = 0;

    SPI_WRITE_TX(SPI0, (uint8_t)command);
    while(SPI_IS_BUSY(SPI0));

    SPI_WRITE_TX(SPI0, 0);
    while(SPI_IS_BUSY(SPI0));
    status = SPI_READ_RX(SPI0);

    for(size_t i = 0; i < (size_t)size; i++) {
        SPI_WRITE_TX(SPI0, 0);
        while(SPI_IS_BUSY(SPI0));
        buffer[i] = SPI_READ_RX(SPI0);
    }

    // RF_NSS
    PA3 = 1;

    SX126xWaitOnBusy();

    return status;
}

void SX126xWriteRegisters(uint16_t address, uint8_t *buffer, uint16_t size) {
    SX126xCheckDeviceReady();

    // RF_NSS
    PA3 = 0;
    
    SPI_WRITE_TX(SPI0, RADIO_WRITE_REGISTER);
    while(SPI_IS_BUSY(SPI0));
    SPI_WRITE_TX(SPI0, (address & 0xFF00) >> 8);
    while(SPI_IS_BUSY(SPI0));
    SPI_WRITE_TX(SPI0, (address & 0x00FF) >> 0);
    while(SPI_IS_BUSY(SPI0));

    for(size_t i = 0; i < (size_t)size; i++) {
        SPI_WRITE_TX(SPI0, buffer[i]);
        while(SPI_IS_BUSY(SPI0));
    }

    // RF_NSS
    PA3 = 1;

    SX126xWaitOnBusy();
}

void SX126xWriteRegister(uint16_t address, uint8_t value) {
    SX126xWriteRegisters(address, &value, 1);
}

void SX126xReadRegisters(uint16_t address, uint8_t *buffer, uint16_t size) {
    SX126xCheckDeviceReady();

    // RF_NSS
    PA3 = 0;
    
    SPI_WRITE_TX(SPI0, RADIO_READ_REGISTER);
    while(SPI_IS_BUSY(SPI0));
    SPI_WRITE_TX(SPI0, (address & 0xFF00) >> 8);
    while(SPI_IS_BUSY(SPI0));
    SPI_WRITE_TX(SPI0, (address & 0x00FF) >> 0);
    while(SPI_IS_BUSY(SPI0));

    SPI_WRITE_TX(SPI0, 0);
    while(SPI_IS_BUSY(SPI0));

    for(size_t i = 0; i < (size_t)size; i++) {
        SPI_WRITE_TX(SPI0, 0);
        while(SPI_IS_BUSY(SPI0));
        buffer[i] = SPI_READ_RX(SPI0);
    }

    // RF_NSS
    PA3 = 1;

    SX126xWaitOnBusy();
}

uint8_t SX126xReadRegister(uint16_t address) {
    uint8_t data;
    SX126xReadRegisters(address, &data, 1);
    return data;
}

void SX126xWriteBuffer(uint8_t offset, uint8_t *buffer, uint8_t size)
{
    SX126xCheckDeviceReady();

    // RF_NSS
    PA3 = 0;

    SPI_WRITE_TX(SPI0, RADIO_WRITE_BUFFER);
    while(SPI_IS_BUSY(SPI0));
    SPI_WRITE_TX(SPI0, offset);
    while(SPI_IS_BUSY(SPI0));

    for(size_t i = 0; i < (size_t)size; i++) {
        SPI_WRITE_TX(SPI0, buffer[i]);
        while(SPI_IS_BUSY(SPI0));
    }

    // RF_NSS
    PA3 = 1;

    SX126xWaitOnBusy();
}

void SX126xReadBuffer(uint8_t offset, uint8_t *buffer, uint8_t size) {
    SX126xCheckDeviceReady();

    // RF_NSS
    PA3 = 0;

    SPI_WRITE_TX(SPI0, RADIO_READ_BUFFER);
    while(SPI_IS_BUSY(SPI0));
    SPI_WRITE_TX(SPI0, offset);
    while(SPI_IS_BUSY(SPI0));
    SPI_WRITE_TX(SPI0, 0);

    for(size_t i = 0; i < (size_t)size; i++) {
        SPI_WRITE_TX(SPI0, 0);
        while(SPI_IS_BUSY(SPI0));
        buffer[i] = SPI_READ_RX(SPI0);
    }

    // RF_NSS
    PA3 = 1;

    SX126xWaitOnBusy();
}

void SX126xSetRfTxPower(int8_t power) {
    SX126xSetTxParams(power, RADIO_RAMP_40_US);
}

uint8_t SX126xGetDeviceId(void) {
    return SX1261;
}

void SX126xAntSwOn(void) {
    // NOP
}

void SX126xAntSwOff(void) {
    // NOP
}

bool SX126xCheckRfFrequency(uint32_t frequency) {
    return true;
}

uint32_t SX126xGetDio1PinState(void) {
    return PC0;
}

static struct {
    double Time;
} rtcContext;

void RtcInit(void) {
}

uint32_t RtcGetTimerValue(void) {
    return uint32_t(Timeline::SystemTime() * 1000.0);
}

uint32_t RtcSetTimerContext(void) {
    rtcContext.Time = Timeline::SystemTime();
    return uint32_t(rtcContext.Time * 1000.0);
}

uint32_t RtcGetTimerContext(void) {
    return uint32_t(rtcContext.Time * 1000.0);
}

uint32_t RtcGetMinimumTimeout(void) {
    return (uint32_t)(1000.0 / Timeline::effectRate);
}

uint32_t RtcMs2Tick(uint32_t milliseconds) {
    return milliseconds;
}

uint32_t RtcTick2Ms(uint32_t tick) {
    return tick;
}

void RtcDelayMs(uint32_t delay) {
    uint32_t delayTicks = RtcMs2Tick( delay );
    uint32_t refTicks = RtcGetTimerValue();
    while( ( ( RtcGetTimerValue( ) - refTicks ) ) < delayTicks ) {
        __NOP( );
    }
}

static Timeline::Event alarmEvent;

void RtcSetAlarm(uint32_t timeout) {
    alarmEvent.time = Timeline::instance().SystemTime() + (double(timeout) / 1000.0);
    alarmEvent.duration = 0.0;
    if (!Timeline::instance().Scheduled(alarmEvent)) {
        alarmEvent.startFunc = [=](Timeline::Span &) {
            TimerIrqHandler();
        };
        Timeline::instance().Add(alarmEvent);
    }
}

void RtcStopAlarm(void) {
    if (Timeline::instance().Scheduled(alarmEvent)) {
        Timeline::instance().Remove(alarmEvent);
    }
}

uint32_t RtcGetTimerElapsedTime(void) {
    return uint32_t(Timeline::SystemTime() * 1000.0) - uint32_t(rtcContext.Time * 1000.0);;
}

uint32_t RtcGetCalendarTime(uint16_t *milliseconds) {
    double now = Timeline::SystemTime();
    *milliseconds = uint32_t(now) % 1000;
    return uint32_t(now);
}

void RtcBkupWrite(uint32_t data0, uint32_t data1) {
    // Not backed up
}

void RtcBkupRead(uint32_t *data0, uint32_t *data1) {
    // Not backed up
}

void RtcProcess(void) {
    // Not used on this platform.
}

TimerTime_t RtcTempCompensation(TimerTime_t period, float temperature) {
    return 0;
}

const size_t loraWanDataAddr = 0x7E000; // 4KB page

LmnStatus_t EepromMcuWriteBuffer(uint16_t addr, uint8_t *buffer, uint16_t size) {

    SYS_UnlockReg();
    FMC_Open();

    uint32_t *data = reinterpret_cast<uint32_t *>(buffer);
    for (size_t c = 0; c < ( size / sizeof(uint32_t) ); c += sizeof(uint32_t)) {
        FMC_Write(loraWanDataAddr + c, *data++);
    }

    FMC_Close();
    SYS_LockReg();

    return LMN_STATUS_OK;
}

LmnStatus_t EepromMcuReadBuffer(uint16_t addr, uint8_t *buffer, uint16_t size) {

    SYS_UnlockReg();
    FMC_Open();

    uint32_t *oBuf = reinterpret_cast<uint32_t *>(buffer);
    for (size_t c = 0; c < ( size / sizeof(uint32_t) ); c += sizeof(uint32_t)) {
        *oBuf++ = FMC_Read(loraWanDataAddr + c);
    }

    FMC_Close();
    SYS_LockReg();

    return LMN_STATUS_OK;
}

void EepromMcuSetDeviceAddr(uint8_t addr) {
}

LmnStatus_t EepromMcuGetDeviceAddr(void) {
    LmnStatus_t status = LMN_STATUS_ERROR;
    return status;
}

}
