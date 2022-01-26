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

// Nuvonton
#include "M480.h"

// LoraWAN-node
#include "sx126x-board.h"
#include "board.h"
#include "Commissioning.h"

#include "RegionCommon.h"
#include "RegionUS915.h"

#include "LmHandlerMsgDisplay.h"
#include "LmHandler.h"
#include "LmhpCompliance.h"

// std-c
#include <stdio.h>

#define FIRMWARE_VERSION                            0x01030000

#define APP_TX_DUTYCYCLE                            5000
#define APP_TX_DUTYCYCLE_RND                        1000
#define LORAWAN_ADR_STATE                           LORAMAC_HANDLER_ADR_ON
#define LORAWAN_DEFAULT_DATARATE                    DR_0
#define LORAWAN_DEFAULT_CONFIRMED_MSG_STATE         LORAMAC_HANDLER_UNCONFIRMED_MSG
#define LORAWAN_APP_DATA_BUFFER_MAX_SIZE            242
#define LORAWAN_DUTYCYCLE_ON                        true
#define LORAWAN_APP_PORT                            2

typedef enum
{
    LORAMAC_HANDLER_TX_ON_TIMER,
    LORAMAC_HANDLER_TX_ON_EVENT,
} LmHandlerTxEvents_t;

static uint8_t AppDataBuffer[LORAWAN_APP_DATA_BUFFER_MAX_SIZE];

static TimerEvent_t TxTimer;

static void OnMacProcessNotify(void);
static void OnNvmDataChange(LmHandlerNvmContextStates_t state, uint16_t size);
static void OnNetworkParametersChange(CommissioningParams_t* params);
static void OnMacMcpsRequest(LoRaMacStatus_t status, McpsReq_t *mcpsReq, TimerTime_t nextTxIn);
static void OnMacMlmeRequest(LoRaMacStatus_t status, MlmeReq_t *mlmeReq, TimerTime_t nextTxIn);
static void OnJoinRequest(LmHandlerJoinParams_t* params);
static void OnTxData(LmHandlerTxParams_t* params);
static void OnRxData(LmHandlerAppData_t* appData, LmHandlerRxParams_t* params);
static void OnClassChange(DeviceClass_t deviceClass);
static void OnBeaconStatusChange(LoRaMacHandlerBeaconParams_t* params);
static void OnSysTimeUpdate(bool isSynchronized, int32_t timeCorrection);
static void PrepareTxFrame(void);
static void StartTxProcess(LmHandlerTxEvents_t txEvent);
static void UplinkProcess(void);

static void OnTxPeriodicityChanged(uint32_t periodicity);
static void OnTxFrameCtrlChanged(LmHandlerMsgTypes_t isTxConfirmed);
static void OnPingSlotPeriodicityChanged(uint8_t pingSlotPeriodicity);

static void OnTxTimerEvent(void* context);

extern "C" {
static LmHandlerCallbacks_t LmHandlerCallbacks =
{
    .GetBatteryLevel = BoardGetBatteryLevel,
    .GetTemperature = NULL,
    .GetRandomSeed = NULL,
    .OnMacProcess = OnMacProcessNotify,
    .OnNvmDataChange = OnNvmDataChange,
    .OnNetworkParametersChange = OnNetworkParametersChange,
    .OnMacMcpsRequest = OnMacMcpsRequest,
    .OnMacMlmeRequest = OnMacMlmeRequest,
    .OnJoinRequest = OnJoinRequest,
    .OnTxData = OnTxData,
    .OnRxData = OnRxData,
    .OnClassChange= OnClassChange,
    .OnBeaconStatusChange = OnBeaconStatusChange,
    .OnSysTimeUpdate = OnSysTimeUpdate,
};

static LmHandlerParams_t LmHandlerParams =
{
    .Region = ACTIVE_REGION,
    .AdrEnable = LORAWAN_ADR_STATE,
    .IsTxConfirmed = LORAWAN_DEFAULT_CONFIRMED_MSG_STATE,
    .TxDatarate = LORAWAN_DEFAULT_DATARATE,
    .PublicNetworkEnable = LORAWAN_PUBLIC_NETWORK,
    .DutyCycleEnabled = LORAWAN_DUTYCYCLE_ON,
    .DataBufferMaxSize = LORAWAN_APP_DATA_BUFFER_MAX_SIZE,
    .DataBuffer = AppDataBuffer,
    .PingSlotPeriodicity = REGION_COMMON_DEFAULT_PING_SLOT_PERIODICITY,
};

static Version_t fwVersion = {
    .Value = FIRMWARE_VERSION
};

static LmhpComplianceParams_t LmhpComplianceParams = {
    .FwVersion = fwVersion,
    .OnTxPeriodicityChanged = OnTxPeriodicityChanged,
    .OnTxFrameCtrlChanged = OnTxFrameCtrlChanged,
    .OnPingSlotPeriodicityChanged = OnPingSlotPeriodicityChanged,
};
} // extern "C" {

static volatile uint8_t IsMacProcessPending = 0;

static volatile uint8_t IsTxFramePending = 0;

static volatile uint32_t TxPeriodicity = 0;

LoraWAN &LoraWAN::instance() {
    static LoraWAN loraWAN;
    if (!loraWAN.initialized) {
        loraWAN.initialized = true;
        loraWAN.init();
    }
    return loraWAN;
}

void LoraWAN::init() {

    SX126xIoInit();

    if (LmHandlerInit(&LmHandlerCallbacks, &LmHandlerParams) != LORAMAC_HANDLER_SUCCESS) {
        printf("LoRaMac wasn't properly initialized\n");
        // Fatal error, endless loop.
        while (1) {
        }
    }

    LmHandlerSetSystemMaxRxError(20);

    LmHandlerPackageRegister(PACKAGE_ID_COMPLIANCE, &LmhpComplianceParams);

    LmHandlerJoin();

    StartTxProcess(LORAMAC_HANDLER_TX_ON_TIMER);
}

void LoraWAN::update() {
    LmHandlerProcess();
    UplinkProcess();
}

static void OnMacProcessNotify(void)
{
    IsMacProcessPending = 1;
}

static void OnNvmDataChange(LmHandlerNvmContextStates_t state, uint16_t size) {
    DisplayNvmDataChange(state, size);
}

static void OnNetworkParametersChange(CommissioningParams_t* params) {
    DisplayNetworkParametersUpdate(params);
}

static void OnMacMcpsRequest(LoRaMacStatus_t status, McpsReq_t *mcpsReq, TimerTime_t nextTxIn) {
    DisplayMacMcpsRequestUpdate(status, mcpsReq, nextTxIn);
}

static void OnMacMlmeRequest(LoRaMacStatus_t status, MlmeReq_t *mlmeReq, TimerTime_t nextTxIn) {
    DisplayMacMlmeRequestUpdate(status, mlmeReq, nextTxIn);
}

static void OnJoinRequest(LmHandlerJoinParams_t* params) {
    DisplayJoinRequestUpdate(params);
    if(params->Status == LORAMAC_HANDLER_ERROR) {
        LmHandlerJoin();
    } else {
        LmHandlerRequestClass(LORAWAN_DEFAULT_CLASS);
    }
}

static void OnTxData(LmHandlerTxParams_t* params) {
    DisplayTxUpdate(params);
}

static void OnRxData(LmHandlerAppData_t* appData, LmHandlerRxParams_t* params) {
    DisplayRxUpdate(appData, params);
    switch(appData->Port) {
    case 1: // The application LED can be controlled on port 1 or 2
    case LORAWAN_APP_PORT:
        break;
    default:
        break;
    }
}

static void OnClassChange(DeviceClass_t deviceClass) {
    DisplayClassUpdate(deviceClass);
    // Inform the server as soon as possible that the end-device has switched to ClassB
    LmHandlerAppData_t appData = {
        .Port = 0,
        .BufferSize = 0,
        .Buffer = NULL,
    };
    LmHandlerSend(&appData, LORAMAC_HANDLER_UNCONFIRMED_MSG);
}

static void OnBeaconStatusChange(LoRaMacHandlerBeaconParams_t* params) {
    switch(params->State) {
        case LORAMAC_HANDLER_BEACON_RX: {
            break;
        }
        case LORAMAC_HANDLER_BEACON_LOST:
        case LORAMAC_HANDLER_BEACON_NRX: {
            break;
        }
        default: {
            break;
        }
    }

    DisplayBeaconUpdate(params);
}

static void OnSysTimeUpdate(bool isSynchronized, int32_t timeCorrection) {
}

static void PrepareTxFrame(void) {
    if(LmHandlerIsBusy() == true) {
        return;
    }
#if 0
    uint8_t channel = 0;

    AppData.Port = LORAWAN_APP_PORT;

    CayenneLppReset();
    CayenneLppAddDigitalInput(channel++, AppLedStateOn);
    CayenneLppAddAnalogInput(channel++, BoardGetBatteryLevel() * 100 / 254);

    CayenneLppCopy(AppData.Buffer);
    AppData.BufferSize = CayenneLppGetSize();

    if(LmHandlerSend(&AppData, LmHandlerParams.IsTxConfirmed) == LORAMAC_HANDLER_SUCCESS)
    {
        // Switch LED 1 ON
        GpioWrite(&Led1, 1);
        TimerStart(&Led1Timer);
    }
#endif  // #if 0
}

static void StartTxProcess(LmHandlerTxEvents_t txEvent)
{
    switch(txEvent) {
        default:
            // fall-through
        case LORAMAC_HANDLER_TX_ON_TIMER: {
            // Schedule 1st packet transmission
            TimerInit(&TxTimer, OnTxTimerEvent);
            TimerSetValue(&TxTimer, TxPeriodicity);
            OnTxTimerEvent(NULL);
        } break;
        case LORAMAC_HANDLER_TX_ON_EVENT: {
        } break;
    }
}

static void UplinkProcess(void)
{
    uint8_t isPending = 0;
    CRITICAL_SECTION_BEGIN();
    isPending = IsTxFramePending;
    IsTxFramePending = 0;
    CRITICAL_SECTION_END();
    if(isPending == 1) {
        PrepareTxFrame();
    }
}

static void OnTxPeriodicityChanged(uint32_t periodicity)
{
    TxPeriodicity = periodicity;

    if(TxPeriodicity == 0) { // Revert to application default periodicity
        TxPeriodicity = APP_TX_DUTYCYCLE + randr(-APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND);
    }

    // Update timer periodicity
    TimerStop(&TxTimer);
    TimerSetValue(&TxTimer, TxPeriodicity);
    TimerStart(&TxTimer);
}

static void OnTxFrameCtrlChanged(LmHandlerMsgTypes_t isTxConfirmed)
{
    LmHandlerParams.IsTxConfirmed = isTxConfirmed;
}

static void OnPingSlotPeriodicityChanged(uint8_t pingSlotPeriodicity)
{
    LmHandlerParams.PingSlotPeriodicity = pingSlotPeriodicity;
}

static void OnTxTimerEvent(void* context)
{
    TimerStop(&TxTimer);

    IsTxFramePending = 1;

    // Schedule next transmission
    TimerSetValue(&TxTimer, TxPeriodicity);
    TimerStart(&TxTimer);
}
