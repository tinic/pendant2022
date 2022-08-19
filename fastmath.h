
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
static constexpr float fast_rcp(const float x ) {
    float v = std::bit_cast<float>( ( 0xbe6eb3beU - std::bit_cast<uint32_t>(x) ) >> 1 );
    return v * v;
}

__attribute__ ((hot, optimize("Os"), flatten))
static constexpr float fast_rsqrt(const float x) {
    float v = std::bit_cast<float>( 0x5f375a86U - ( std::bit_cast<uint32_t>(x) >> 1 ) );
    return v * (1.5f - ( 0.5f * v * v ));
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
