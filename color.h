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
#ifndef _COLOR_H_
#define _COLOR_H_

#include <cstdint>
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <array>

#include "./vector.h"
#include "./fastmath.h"

namespace color {

    template<class T> struct rgba {

        T r = 0;
        T g = 0;
        T b = 0;
        T a = 0;

        consteval rgba() :
            r(0),
            g(0),
            b(0),
            a(0){
        }

        constexpr rgba(uint32_t);

        constexpr rgba(const rgba &from) :
            r(from.r),
            g(from.g),
            b(from.b),
            a(from.a) {
        }

        constexpr rgba(T _r, T _g, T _b) :
            r(_r),
            g(_g),
            b(_b),
            a(0) {
        }

        constexpr rgba(T _r, T _g, T _b, T _a) :
            r(_r),
            g(_g),
            b(_b),
            a(_a) {
        }

        constexpr rgba(const vector::float4 &from) {
            r = clamp_to_type(from.x);
            g = clamp_to_type(from.y);
            b = clamp_to_type(from.z);
            a = clamp_to_type(from.w);
        }

        rgba<T>& operator=(const rgba<T>& other) = default;

        bool operator!=(const rgba<T>& other) const = default;
        bool operator==(const rgba<T>& other) const = default;

        rgba<T> fix_for_ws2816();

        uint8_t *write_rgba_bytes(uint8_t *dst) {
            *dst++ = r;
            *dst++ = g;
            *dst++ = b;
            *dst++ = a;
            return dst;
        }

    private:
        T clamp_to_type(float v);
    };

    template<> inline rgba<uint16_t> rgba<uint16_t>::fix_for_ws2816() {
        return rgba<uint16_t>(  uint16_t(r < uint16_t(384) ? ( ( r * uint16_t(256) ) / uint16_t(384) ) : r),
                                uint16_t(g < uint16_t(384) ? ( ( g * uint16_t(256) ) / uint16_t(384) ) : g),
                                uint16_t(b < uint16_t(384) ? ( ( b * uint16_t(256) ) / uint16_t(384) ) : b),
                                uint16_t(a < uint16_t(384) ? ( ( a * uint16_t(256) ) / uint16_t(384) ) : a));
    }

    template<> constexpr rgba<uint8_t>::rgba(uint32_t color) {
        r = ( color >> 16 ) & 0xFF;
        g = ( color >>  8 ) & 0xFF;
        b = ( color >>  0 ) & 0xFF;
    }

    template<> constexpr rgba<uint16_t>::rgba(uint32_t color) {
        r = uint16_t(( color >> 16 ) & 0xFF); r |= uint16_t(r << 8);
        g = uint16_t(( color >>  8 ) & 0xFF); g |= uint16_t(g << 8);
        b = uint16_t(( color >>  0 ) & 0xFF); b |= uint16_t(b << 8);
    }

    template<> __attribute__((always_inline)) constexpr float rgba<float>::clamp_to_type(float v) {
        return v;
    }

    template<> __attribute__((always_inline)) constexpr uint8_t rgba<uint8_t>::clamp_to_type(float v) {
        return uint8_t(__builtin_arm_usat(int32_t(v * 255.f), 8));
    }

    template<> __attribute__((always_inline)) constexpr uint16_t rgba<uint16_t>::clamp_to_type(float v) {
        return uint16_t(__builtin_arm_usat(int32_t(v * 65535.f), 16));
    }

    class gradient {
    public:
        template<class T, std::size_t N> constexpr gradient(const T (&stops)[N]) {
            for (size_t c = 0; c < colors_n; c++) {
                float f = static_cast<float>(c) / static_cast<float>(colors_n - 1);
                vector::float4 a = stops[0];
                vector::float4 b = stops[1];
                if (N > 2) {
                    for (int32_t d = static_cast<int32_t>(N-2); d >= 0 ; d--) {
                        if ( f >= (stops[d].w) ) {
                            a = stops[d+0];
                            b = stops[d+1];
                            break;
                        }
                    }
                }
                f -= a.w;
                f /= b.w - a.w;
                colors[c] = a.lerp(b,f);
            }
        }

        vector::float4 repeat(float i) const;
        vector::float4 reflect(float i) const;
        vector::float4 clamp(float i) const;

    private:
        static constexpr size_t colors_n = 256;
        static constexpr float colors_mul = 255.0;
        static constexpr size_t colors_mask = 0xFF;

        vector::float4 colors[colors_n];
    };

    class convert {
    public:

        constexpr convert() : sRGB2lRGB() {
            for (size_t c = 0; c < 256; c++) {
                float v = float(c) / 255.0f;
                if (v > 0.04045f) {
                    sRGB2lRGB[c] = constexpr_pow( (v + 0.055f) / 1.055f, 2.4f);
                } else {
                    sRGB2lRGB[c] = v * ( 25.0f / 323.0f );
                };
            }
        }

        constexpr vector::float4 sRGB2CIELUV(const rgba<uint8_t> &in) const  {
            float r = sRGB2lRGB[in.r];
            float g = sRGB2lRGB[in.g];
            float b = sRGB2lRGB[in.b];

            float X = 0.4124564f * r + 0.3575761f * g + 0.1804375f * b;
            float Y = 0.2126729f * r + 0.7151522f * g + 0.0721750f * b;
            float Z = 0.0193339f * r + 0.1191920f * g + 0.9503041f * b;

            const float wu = 0.197839825f;
            const float wv = 0.468336303f;

            float l = ( Y <= 0.008856452f ) ? ( 9.03296296296f * Y) : ( 1.16f * constexpr_pow(Y, 1.0f / 3.0f) - 0.16f);
            float d = X + 15.f * Y + 3.0f * Z;
            float di = fast_rcp(d);

            return vector::float4(l,
                (d > (1.0f / 65536.0f)) ? 13.0f * l * ( ( 4.0f * X * di ) - wu ) : 0.0f,
                (d > (1.0f / 65536.0f)) ? 13.0f * l * ( ( 9.0f * Y * di ) - wv ) : 0.0f);
        }

        constexpr vector::float4 sRGB2OKLAB(const rgba<uint8_t> &in) const  {
            float r = sRGB2lRGB[in.r];
            float g = sRGB2lRGB[in.g];
            float b = sRGB2lRGB[in.b];

            float l = 0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b;
            float m = 0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b;
            float s = 0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b;

            float l_ = fast_cbrtf(l);
            float m_ = fast_cbrtf(m);
            float s_ = fast_cbrtf(s);

            return vector::float4(
                0.2104542553f*l_ + 0.7936177850f*m_ - 0.0040720468f*s_,
                1.9779984951f*l_ - 2.4285922050f*m_ + 0.4505937099f*s_,
                0.0259040371f*l_ + 0.7827717662f*m_ - 0.8086757660f*s_
            );
        }

        vector::float4 CIELUV2sRGB(const vector::float4 &) const;
        vector::float4 CIELUV2LED(const vector::float4 &) const;

        vector::float4 OKLAB2sRGB(const vector::float4 &) const;
        vector::float4 OKLAB2LED(const vector::float4 &) const;

    private:
        float sRGB2lRGB[256];
    };

    constexpr vector::float4 lRGB2CIELUV(const vector::float4 &in) {
        float r = in.x;
        float g = in.y;
        float b = in.z;

        float X = 0.4124564f * r + 0.3575761f * g + 0.1804375f * b;
        float Y = 0.2126729f * r + 0.7151522f * g + 0.0721750f * b;
        float Z = 0.0193339f * r + 0.1191920f * g + 0.9503041f * b;

        const float wu = 0.197839825f;
        const float wv = 0.468336303f;

        float l = ( Y <= 0.008856452f ) ? ( 9.03296296296f * Y) : ( 1.16f * constexpr_pow(Y, 1.0f / 3.0f) - 0.16f);
        float d = X + 15.f * Y + 3.0f * Z;
        float di = fast_rcp(d);

        return vector::float4(l,
            (d > (1.0f / 65536.0f)) ? 13.0f * l * ( ( 4.0f * X * di ) - wu ) : 0.0f,
            (d > (1.0f / 65536.0f)) ? 13.0f * l * ( ( 9.0f * Y * di ) - wv ) : 0.0f, in.w);
    }

    constexpr vector::float4 sRGB2CIELUV(const vector::float4 &in) {

        auto sRGB2lRGB = [](float v) {
            if (v > 0.04045f) {
                return constexpr_pow( (v + 0.055f) / 1.055f, 2.4f);
            } else {
                return v * ( 25.0f / 323.0f );
            };
        };

        float r = sRGB2lRGB(in.x);
        float g = sRGB2lRGB(in.y);
        float b = sRGB2lRGB(in.z);

        float X = 0.4124564f * r + 0.3575761f * g + 0.1804375f * b;
        float Y = 0.2126729f * r + 0.7151522f * g + 0.0721750f * b;
        float Z = 0.0193339f * r + 0.1191920f * g + 0.9503041f * b;

        const float wu = 0.197839825f;
        const float wv = 0.468336303f;

        float l = ( Y <= 0.008856452f ) ? ( 9.03296296296f * Y) : ( 1.16f * constexpr_pow(Y, 1.0f / 3.0f) - 0.16f);
        float d = X + 15.f * Y + 3.0f * Z;
        float di = fast_rcp(d);

        return vector::float4(l,
            (d > (1.0f / 65536.0f)) ? 13.0f * l * ( ( 4.0f * X * di ) - wu ) : 0.0f,
            (d > (1.0f / 65536.0f)) ? 13.0f * l * ( ( 9.0f * Y * di ) - wv ) : 0.0f, in.w);
    }

    constexpr vector::float4 lRGB2OKLAB(const vector::float4 &in) {
        float r = in.x;
        float g = in.y;
        float b = in.z;

        float l = 0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b;
        float m = 0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b;
        float s = 0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b;

        float l_ = fast_cbrtf(l);
        float m_ = fast_cbrtf(m);
        float s_ = fast_cbrtf(s);

        return vector::float4(
            0.2104542553f*l_ + 0.7936177850f*m_ - 0.0040720468f*s_,
            1.9779984951f*l_ - 2.4285922050f*m_ + 0.4505937099f*s_,
            0.0259040371f*l_ + 0.7827717662f*m_ - 0.8086757660f*s_
        );
    }

    constexpr vector::float4 sRGB2OKLAB(const vector::float4 &in) {

        auto sRGB2lRGB = [](float v) {
            if (v > 0.04045f) {
                return constexpr_pow( (v + 0.055f) / 1.055f, 2.4f);
            } else {
                return v * ( 25.0f / 323.0f );
            };
        };

        float r = sRGB2lRGB(in.x);
        float g = sRGB2lRGB(in.y);
        float b = sRGB2lRGB(in.z);

        float l = 0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b;
        float m = 0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b;
        float s = 0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b;

        float l_ = fast_cbrtf(l);
        float m_ = fast_cbrtf(m);
        float s_ = fast_cbrtf(s);

        return vector::float4(
            0.2104542553f*l_ + 0.7936177850f*m_ - 0.0040720468f*s_,
            1.9779984951f*l_ - 2.4285922050f*m_ + 0.4505937099f*s_,
            0.0259040371f*l_ + 0.7827717662f*m_ - 0.8086757660f*s_
        );
    }

    constexpr vector::float4 hsv(const vector::float4 &hsv) {
        float h = hsv.x;
        float s = hsv.y;
        float v = hsv.z;

        int32_t rd = static_cast<int32_t>( 6.0f * h );

        float f = h * 6.0f - float(rd);
        float p = v * (1.0f - s);
        float q = v * (1.0f - f * s);
        float t = v * (1.0f - (1.0f - f) * s);

        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;

        switch ( rd % 6 ) {
            default:
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: r = v; g = p; b = q; break;
        }

        return vector::float4(lRGB2OKLAB(vector::float4(r,g,b,hsv.w)));
    }

    constexpr vector::float4 srgb(const vector::float4 &color, float alpha = 1.0f) {
        return vector::float4(sRGB2OKLAB(color), alpha);
    }

    constexpr vector::float4 srgb8(const rgba<uint8_t> &color, float alpha = 1.0f) {
        return vector::float4(convert().sRGB2OKLAB(color), alpha);
    }

    constexpr vector::float4 srgb8_stop(const rgba<uint8_t> &color, float stop) {
        return vector::float4(convert().sRGB2OKLAB(color), stop);
    }

    constexpr vector::float4 srgb8_stop(uint32_t color, float stop) {
        return vector::float4(convert().sRGB2OKLAB(rgba<uint8_t>(color)), stop);
    }
}

namespace vector {
    constexpr float4::float4(color::rgba<uint8_t> color) {
        this->x = float(color.r)*(1.0f/255.0f);
        this->y = float(color.g)*(1.0f/255.0f);
        this->z = float(color.b)*(1.0f/255.0f);
        this->w = 1.0f;
    }
}

#endif  // #ifndef _COLOR_H_
