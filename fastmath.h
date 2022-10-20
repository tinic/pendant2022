
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
#ifndef FASTMATH_H_
#define FASTMATH_H_

#include <stdint.h>
#include <math.h>
#include <bit>

__attribute__ ((hot, optimize("Os"), flatten))
static constexpr float fastest_rcp(const float x ) { // 8bits of accuracy
    float v = std::bit_cast<float>( ( 0xbe6eb3beU - std::bit_cast<uint32_t>(x) ) >> 1 );
    return v * v;
}

__attribute__ ((hot, optimize("Os"), flatten))
static constexpr float fastest_rsqrt(const float x) { // 8bits of accuracy
    float v = std::bit_cast<float>( 0x5f375a86U - ( std::bit_cast<uint32_t>(x) >> 1 ) );
    return v * (1.5f - ( 0.5f * v * v ));
}

__attribute__ ((hot, optimize("Os"), flatten))
static constexpr float fast_rcp(const float x) { // 23.8bits of accuracy
    int32_t i = std::bit_cast<int>(x);
    i = 0x7eb53567 - i;
    float y = std::bit_cast<float>(i);
    y = 1.9395974f * y * ( 1.436142f + y * -x);
    y = y + y * (1.0f + y * -x);
    return y;
}

__attribute__ ((hot, optimize("Os"), flatten))
static constexpr float fast_exp2(const float p) {
    const float offset = (p < 0) ? 1.0f : 0.0f;
    const float clipp = (p < -126) ? -126.0f : p;
    const float z = clipp - static_cast<float>(static_cast<int32_t>(clipp)) + offset;
    return std::bit_cast<float>(static_cast<uint32_t>((1 << 23) * (clipp + 121.2740575f + 27.7280233f / (4.84252568f - z) - 1.49012907f * z)));
}

__attribute__ ((hot, optimize("Os"), flatten))
static constexpr float fast_log2(const float x) {
    uint32_t xi = std::bit_cast<uint32_t>(x);
    float xf = std::bit_cast<float>((xi & 0x007FFFFF) | 0x3f000000);
    const float y = static_cast<float>(xi) * 1.1920928955078125e-7f;
    return y - 124.22551499f
             - 1.498030302f * xf
             - 1.72587999f / (0.3520887068f + xf);
}

__attribute__ ((hot, optimize("Os"), flatten))
static constexpr float fast_cbrtf(const float x) { // 21bits of accuracy
    float k1 = 1.7523196760f;
    float k2 = 1.2509524245f;
    float k3 = 0.5093818292f;
    int32_t i = std::bit_cast<int>(x);
    i = 0x548c2b4b - i / 3;
    float y = std::bit_cast<float>(i);
    float c = x * y * y * y;
    y = y * (k1 - c * (k2 - k3 * c));
    c = 1.0f - x * y * y * y;
    y = y * (1.0f + 0.333333333333f * c);
    return fast_rcp(y);
}

__attribute__ ((hot, optimize("Os"), flatten))
static constexpr float fastest_cbrtf(const float x) { // 10bits of accuracy
    int32_t i = std::bit_cast<int>(x);
    i = 0x548c39cb - i / 3;
    float y = std::bit_cast<float>(i);
    y = y * (1.5015480449f - 0.534850249f * x * y * y * y);
    y = y * (1.3333339850f - 0.333333330f * x * y * y * y);;
    return fast_rcp(y);
}

__attribute__ ((hot, optimize("Os"), flatten))
static constexpr float fast_log(const float x) {
    return fast_log2(x) * 0.69314718f;
}

__attribute__ ((hot, optimize("Os"), flatten))
static constexpr float fast_pow(const float x, const float p) {
    return fast_exp2(p * fast_log2(x));
}

__attribute__ ((hot, optimize("Os"), flatten))
static constexpr double frac(double v) { // same as fmod(v, 1.0)
    return v - std::trunc(v);
}

__attribute__ ((hot, optimize("Os"), flatten))
static constexpr float fracf(float v) { // same as fmodf(v, 1.0f)
    return v - std::truncf(v);
}

__attribute__ ((hot, optimize("Os"), flatten))
static constexpr float constexpr_pow(const float x, const float p) {
    return ::exp2f(p * ::log2f(x));
}

__attribute__ ((hot, optimize("Os"), flatten))
static constexpr float signf(float x) {
	return (x > 0.0f) ? 1.0f : ( (x < 0.0f) ? -1.0f : 1.0f);
}

#endif  // #ifndef FASTMATH_H_
