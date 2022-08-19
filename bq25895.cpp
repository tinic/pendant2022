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
#include "./bq25895.h"
#include "./i2cmanager.h"

#include <stdio.h>

bool BQ25895::devicePresent = false;

BQ25895  &BQ25895 ::instance() {
    static BQ25895 bq25895;
    if (!bq25895.initialized) {
        bq25895.initialized = true;
        bq25895.init();
    }
    return bq25895;
}

void BQ25895::DisableWatchdog() {
    if (!devicePresent) return;
    i2c1::instance().clearReg8Bits(i2c_addr, 0x07, (1 << 4));
    i2c1::instance().clearReg8Bits(i2c_addr, 0x07, (1 << 5));
}
     
void BQ25895::DisableOTG() {
    if (!devicePresent) return;
    i2c1::instance().clearReg8Bits(i2c_addr, 0x03, (1 << 5));
}

void BQ25895::EnableOTG() {
    if (!devicePresent) return;
    i2c1::instance().setReg8Bits(i2c_addr, 0x03, (1 << 5));
}

void BQ25895::SetMinSystemVoltage (uint32_t voltageMV) {
    uint8_t reg =i2c1::instance().getReg8(i2c_addr, 0x03);
    if ((voltageMV >= 3000) && (voltageMV <= 3700)) {
        uint32_t codedValue = voltageMV;
        codedValue = (codedValue - 3000) / 100;
        reg &= uint8_t(~(0x07 << 1));
        reg |= static_cast<uint8_t>((codedValue & 0x07) << 1);
        i2c1::instance().setReg8(i2c_addr, 0x03, reg);
    }
}

void BQ25895::SetBoostVoltage (uint32_t voltageMV) {
    if (!devicePresent) return;
    uint8_t reg = i2c1::instance().getReg8(i2c_addr, 0x0A);
    if ((voltageMV >= 4550) && (voltageMV <= 5510)) {
        uint32_t codedValue = voltageMV;
        codedValue = (codedValue - 4550) / 64;
        if ((voltageMV - 4550) % 64 != 0) {
            codedValue++;
        }
        reg &= uint8_t(~(0x0f << 4));
        reg |= static_cast<uint8_t>((codedValue & 0x0f) << 4);
        i2c1::instance().setReg8(i2c_addr, 0x0A, reg);
    }
}

uint32_t BQ25895::GetBoostVoltage() {
    if (!devicePresent) return 0;
    uint8_t reg = i2c1::instance().getReg8(i2c_addr, 0x0A);
    reg = (reg >> 4) & 0x0f;
    return 4550 + (static_cast<uint32_t>(reg)) * 64;
}

void BQ25895::SetInputCurrent(uint32_t currentMA) {
    if (!devicePresent) return;
    if (currentMA >= 50 && currentMA <= 3250) {
        uint32_t codedValue = currentMA;
        codedValue = ((codedValue) / 50) - 1;
        i2c1::instance().setReg8(i2c_addr, 0x00, static_cast<uint8_t>(codedValue));
    }
    if (currentMA == 0) {
        uint32_t codedValue = 0;
        i2c1::instance().setReg8(i2c_addr,0x00, static_cast<uint8_t>(codedValue));
    }
}

void BQ25895::SetFastChargeCurrent(uint32_t currentMA) {
    if (!devicePresent) return;
    if (currentMA >= 64 && currentMA <= 5056) {
        uint32_t codedValue = currentMA;
        codedValue = ((codedValue) / 64);
        i2c1::instance().setReg8(i2c_addr, 0x04, static_cast<uint8_t>(codedValue));
    }
    if (currentMA == 0) {
        uint32_t codedValue = 0;
        i2c1::instance().setReg8(i2c_addr, 0x04, static_cast<uint8_t>(codedValue));
    }
} 

bool BQ25895::ADCActive() {
    if (!devicePresent) return false;
    return ( i2c1::instance().getReg8(i2c_addr, 0x02) & (1 << 7) ) ? true : false;
}

void BQ25895::OneShotADC() {
    if (!devicePresent) return;
    i2c1::instance().clearReg8Bits(i2c_addr, 0x02, (1 << 6));
    i2c1::instance().setReg8Bits(i2c_addr, 0x02, (1 << 7));
}

uint32_t BQ25895::GetInputCurrent () {
    if (!devicePresent) return 0;
    return ((i2c1::instance().getReg8(i2c_addr, 0x00) & (0x3F)) * 50);
}

uint32_t BQ25895::GetFastChargeCurrent () {
    if (!devicePresent) return 0;
    return ((i2c1::instance().getReg8(i2c_addr, 0x04) & (0x7F)) * 64);
}

void BQ25895::ForceDPDMDetection() {
    if (!devicePresent) return;
    i2c1::instance().setReg8Bits(i2c_addr, 0x02, (1 << 1));
}

void BQ25895::update() {
    if (!devicePresent) return;
    statusRaw = i2c1::instance().getReg8(i2c_addr, 0x0B);
    faultStateRaw = i2c1::instance().getReg8(i2c_addr, 0x0C);
    batteryVoltageRaw = i2c1::instance().getReg8(i2c_addr,0x0E) & 0x7F;
    systemVoltageRaw = i2c1::instance().getReg8(i2c_addr,0x0F) & 0x7F;
    vbusVoltageRaw = i2c1::instance().getReg8(i2c_addr,0x11) & 0x7F;
    chargeCurrentRaw = i2c1::instance().getReg8(i2c_addr,0x12) & 0x7F;
    OneShotADC();
}

void BQ25895::stats() {
    printf("Battery Voltage: %g\r\n", double(BatteryVoltage()));
    printf("System Voltage: %g\r\n", double(SystemVoltage()));
    printf("VBUSVoltage Voltage: %g\r\n", double(VBUSVoltage()));
    printf("Charge Current: %g\r\n", double(ChargeCurrent()));
    printf("Status: VBUS(%02x) Charge(%02x)\r\n", VBUSStatus(), ChargeStatus());
}

void BQ25895::init() {
    if (!devicePresent) return;
    DisableOTG();
    DisableWatchdog();
    OneShotADC();
    SetBoostVoltage(4550);
    SetMinSystemVoltage(3500);
    SetInputCurrent(1000);
    SetFastChargeCurrent(1000);
    ForceDPDMDetection();
    update();
    stats();
} 
