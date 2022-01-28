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
#include "./ens210.h"
#include "./i2cmanager.h"

#include <stdio.h>

static void delay_us(int usec) {
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

bool ENS210::devicePresent = false;

ENS210 &ENS210::instance() {
    static ENS210 ens210;
    if (!ens210.initialized) {
        ens210.initialized = true;
        ens210.init();
    }
    return ens210;
}

void ENS210::reset() {
    if (!devicePresent) return;
    static uint8_t th_reset[] = { 0x10, 0x80 };
    i2c1::instance().write(i2c_addr, th_reset, 2);
    delay_us(2000);
    static uint8_t th_normal[] = { 0x10, 0x00 };
    i2c1::instance().write(i2c_addr, th_normal, 2);
    delay_us(2000);
}

void ENS210::measure() {
    if (!devicePresent) return;
    static uint8_t th_start_single[] = { 0x21, 0x00, 0x03 };
    i2c1::instance().write(i2c_addr, (uint8_t *)&th_start_single, sizeof(th_start_single));
}

void ENS210::wait() {
    if (!devicePresent) return;
    static uint8_t th_sens_stat = 0x24;
    static uint8_t th_stat = 0;
    do {
        i2c1::instance().write(i2c_addr, (uint8_t *)&th_sens_stat, sizeof(th_sens_stat));
        i2c1::instance().read(i2c_addr, (uint8_t *)&th_stat, sizeof(th_stat));
        delay_us(2000);
    } while (th_stat);
}

void ENS210::read() {
    if (!devicePresent) return;
    static uint8_t th_read = 0x30;
    static uint8_t th_data[6] = { 0, 0, 0, 0, 0, 0 };
    i2c1::instance().write(i2c_addr, (uint8_t *)&th_read, sizeof(th_read));
    i2c1::instance().read(i2c_addr, (uint8_t *)&th_data, sizeof(th_data));

    uint32_t t_val = (uint32_t(th_data[2])<<16U) + (uint32_t(th_data[1])<<8U) + (uint32_t(th_data[0])<<0U);
    uint32_t h_val = (uint32_t(th_data[5])<<16U) + (uint32_t(th_data[4])<<8U) + (uint32_t(th_data[3])<<0U);

    uint32_t t_data = (t_val>>0 ) & 0xffff;
    float TinK = (float)t_data / 64;
    float TinC = TinK - 273.15f;
    temperature = TinC;
    temperatureRaw = static_cast<uint16_t>(t_data);

    uint32_t h_data = (h_val>>0 ) & 0xffff;
    float H = (float)h_data/51200;
    humidity = H;
    humidityRaw = static_cast<uint16_t>(h_data);
}

void ENS210::update() {
    if (!devicePresent) return;
    read();
    measure();
}

void ENS210::init() {
    if (!devicePresent) return;
    reset();
    measure();
    wait();
    read();
    stats();
}

void ENS210::stats() {
    printf("Temperature: %g\r\n", double(Temperature()));
    printf("Humidity: %g\r\n", double(Humidity()));
}
