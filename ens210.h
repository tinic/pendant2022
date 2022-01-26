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
#ifndef ENS210_H_
#define ENS210_H_

#include <stdint.h>

class ENS210 {
public:
    static ENS210 &instance();

    void update();
    void stats();

    float Temperature() const { return temperature; }
    float Humidity() const { return humidity; }

private:
    friend class i2c1;
    friend class i2c2;
    
    static constexpr uint8_t i2c_addr = 0x43;
    static constexpr const char *str_id = "ENS210";
    static bool devicePresent;

    float temperature;
    float humidity;

    uint16_t temperatureRaw;
    uint16_t humidityRaw;

    void reset();
    void read();
    void measure();
    void wait();

    void init();
    bool initialized = false;
};

#endif /* ENS210_H_ */