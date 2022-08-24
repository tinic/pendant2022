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
#ifndef BQ25895_H_
#define BQ25895_H_

#include <stdint.h>

class BQ25895 {
public:
    static BQ25895 &instance();

    void update();

    uint8_t Status() const { return statusRaw; }
    uint8_t FaultState() const { return faultStateRaw; }

    float BatteryVoltage() const { return 2.304f + ( static_cast<float>(batteryVoltageRaw) * 2.540f ) * ( 1.0f / 127.0f); }
    float SystemVoltage() const { return 2.304f + ( static_cast<float>(systemVoltageRaw) * 2.540f ) * ( 1.0f / 127.0f); }
    float VBUSVoltage() const { return 2.6f + ( static_cast<float>(vbusVoltageRaw) * 12.7f ) * ( 1.0f / 127.0f); }
    float ChargeCurrent() const { return ( static_cast<float>(chargeCurrentRaw) * 6350.0f ) * ( 1.0f / 127.0f); }

    static constexpr float MinBatteryVoltage() { return 3.5f; }
    static constexpr float MaxBatteryVoltage() { return 4.2f; }

    enum {
        VBUS_NO_INPUT       = 0b000,
        VBUS_USB_SDP        = 0b001,
        VBUS_USB_CDP        = 0b010,
        VBUS_USB_DCP        = 0b011,
        VBUS_ADJ_DCP        = 0b100,
        VBUS_UNKOWN         = 0b101,
        VBUS_NON_STANDARD   = 0b110,
        VBUS_OTG            = 0b111
    };

    uint8_t VBUSStatus() const { return (statusRaw >> 5) & 0x7; }

    enum {
        NOT_CHARGING        = 0b00,
        PRE_CHARGING        = 0b01,
        FAST_CHARGING       = 0b10,
        TERM_CHARGING       = 0b11,
    };

    uint8_t ChargeStatus() const { return (statusRaw >> 3) & 0x3; }

private:
    friend class i2c1;
    friend class i2c2;

    static constexpr uint8_t i2c_addr = 0x6a;
    static constexpr const char *str_id = "BQ25895";
    static bool devicePresent;

    uint8_t batteryVoltageRaw = 0;
    uint8_t systemVoltageRaw = 0;
    uint8_t vbusVoltageRaw = 0;
    uint8_t chargeCurrentRaw = 0;
    uint8_t statusRaw = 0;
    uint8_t faultStateRaw = 0;

    void DisableWatchdog();
    void DisableOTG();
    void EnableOTG();
    void ForceDPDMDetection();

    bool ADCActive();
    void OneShotADC();

    void SetInputCurrent(uint32_t currentMA);
    uint32_t GetInputCurrent();

    void SetFastChargeCurrent(uint32_t currentMA);
    uint32_t GetFastChargeCurrent();

    void SetBoostVoltage (uint32_t voltageMV);
    uint32_t GetBoostVoltage();

    void SetMinSystemVoltage (uint32_t voltageMV);

    void stats();

    void init();
    bool initialized = false;
};

#endif /* BQ25895_H_ */