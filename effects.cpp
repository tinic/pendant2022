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
#include "./effects.h"
#include "./timeline.h"
#include "./leds.h"
#include "./model.h"
#include "./color.h"
#include "./fastmath.h"
#include "./seed.h"
#include "./mmc5633njl.h"

#include <random>
#include <array>
#include <limits>
#include <math.h>

static constexpr color::gradient gradient_rainbow({
    color::srgb8_stop({0xff,0x00,0x00}, 0.00f),
    color::srgb8_stop({0xff,0xff,0x00}, 0.16f),
    color::srgb8_stop({0x00,0xff,0x00}, 0.33f),
    color::srgb8_stop({0x00,0xff,0xff}, 0.50f),
    color::srgb8_stop({0x00,0x00,0xff}, 0.66f),
    color::srgb8_stop({0xff,0x00,0xff}, 0.83f),
    color::srgb8_stop({0xff,0x00,0x00}, 1.00f)
});

Effects &Effects::instance() {
    static Effects effects;
    if (!effects.initialized) {
        effects.initialized = true;
        effects.init();
    }
    return effects;
}

void Effects::black() {
/*
    memset(leds_centr, 0, sizeof(leds_centr));
    memset(leds_outer, 0, sizeof(leds_outer));
    memset(leds_inner, 0, sizeof(leds_inner));
*/
}

void Effects::standard_bird() {
    vector::float4 bird(color::srgb8(Model::instance().BirdColor()));

    auto calc = [](const std::function<vector::float4 ()> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::birdLedsN; c++) {
            auto col = func();
            leds.setBird(0,c,col);
            leds.setBird(1,c,col);
        }
    };

    calc([=]() {
        return bird;
    });
}

void Effects::color_walker() {
    standard_bird();

    double now = Timeline::SystemTime();

    const double speed = 1.0;

    float rgb_walk = (       static_cast<float>(frac(now * (1.0 / 5.0) * speed)));
    float val_walk = (1.0f - static_cast<float>(frac(now               * speed)));

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos, float walk)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            float mod_walk = fracf(val_walk + (1.0f - (float(c) * ( 1.0f / static_cast<float>(Leds::circleLedsN)))));
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos, mod_walk);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    vector::float4 col(gradient_rainbow.repeat(rgb_walk));
    calc([=](const vector::float4 &pos, float walk) {
        float v = fast_pow(std::min(1.0f, walk), 2.0f);;
        return col * v;
    });

}

void Effects::light_walker() {
    standard_bird();

    double now = Timeline::SystemTime();

    const double speed = 1.0;

    float rgb_walk = (       static_cast<float>(frac(now * (1.0 / 5.0) * speed)));
    float val_walk = (1.0f - static_cast<float>(frac(now               * speed)));

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos, float walk)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            float mod_walk = fracf(val_walk + (1.0f - (float(c) * ( 1.0f / static_cast<float>(Leds::circleLedsN)))));
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos, mod_walk);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };
    calc([=](const vector::float4 &pos, float walk) {
        return color::hsv({rgb_walk, 1.0f - fast_pow(std::min(1.0f, walk), 6.0f), fast_pow(std::min(1.0f, walk), 6.0f)});
    });
}

template<const std::size_t n> static void band_mapper(std::array<float, n> &stops, float stt, float end) {

    for (; stt < 0 ;) {
        stt += 1.0f;
        end += 1.0f;
    }

    for (; stt > 1.0f ;) {
        stt -= 1.0f;
        end -= 1.0f;
    }

    stt = fmodf(stt, 2.0f) + 0.5f;
    end = fmodf(end, 2.0f) + 0.5f;

    float stop_step = static_cast<float>(stops.size());
    float stop_stepi = 1.0f / static_cast<float>(stops.size());

    float stop_stt = -stop_stepi * 0.5f + 0.5f;
    float stop_end = +stop_stepi * 0.5f + 0.5f;

    for (size_t c = 0; c < stops.size(); c++) {
        stops[c] = 0.0f;
    }

    for (size_t c = 0; c < stops.size() * 2; c++) {
        if (stt <= stop_stt && end >= stop_end) {
            if ( ( stop_stt - stt ) < stop_stepi) {
                stops[c % stops.size()] = ( stop_stt - stt ) * stop_step;
            } else if ( ( end - stop_end ) < stop_stepi) {
                stops[c % stops.size()] = ( end - stop_end ) * stop_step;
            } else {
                stops[c % stops.size()] = 1.0f;
            }
        }
        stop_end += stop_stepi;
        stop_stt += stop_stepi;
    }
}

void Effects::rgb_band() {

    standard_bird();

    static float rgb_band_r_walk = 0.0f;
    static float rgb_band_g_walk = 0.0f;
    static float rgb_band_b_walk = 0.0f;

    static float rgb_band_r_walk_step = 1.0f;
    static float rgb_band_g_walk_step = 1.0f;
    static float rgb_band_b_walk_step = 1.0f;

    static std::mt19937 gen;
    static std::uniform_real_distribution<float> disf(+0.001f, +0.005f);
    static std::uniform_int_distribution<int32_t> disi(0, 1);

    std::array<float, Leds::circleLedsN> band_r;
    std::array<float, Leds::circleLedsN> band_g;
    std::array<float, Leds::circleLedsN> band_b;

    if (fabsf(rgb_band_r_walk) >= 2.0f) {
        while (rgb_band_r_walk >= +1.0f) { rgb_band_r_walk -= 1.0f; }
        while (rgb_band_r_walk <= -1.0f) { rgb_band_r_walk += 1.0f; }
        rgb_band_r_walk_step = disf(gen) * (disi(gen) ? 1.0f : -1.0f);
    }

    if (fabsf(rgb_band_g_walk) >= 2.0f) {
        while (rgb_band_g_walk >= +1.0f) { rgb_band_g_walk -= 1.0f; }
        while (rgb_band_g_walk <= -1.0f) { rgb_band_g_walk += 1.0f; }
        rgb_band_g_walk_step = disf(gen) * (disi(gen) ? 1.0f : -1.0f);
    }

    if (fabsf(rgb_band_b_walk) >= 2.0f) {
        while (rgb_band_b_walk >= +1.0f) { rgb_band_b_walk -= 1.0f; }
        while (rgb_band_b_walk <= -1.0f) { rgb_band_b_walk += 1.0f; }
        rgb_band_b_walk_step = disf(gen) * (disi(gen) ? 1.0f : -1.0f);
    }

    band_mapper(band_r, rgb_band_r_walk, rgb_band_r_walk + (1.0f / 3.0f));
    band_mapper(band_g, rgb_band_g_walk, rgb_band_g_walk + (1.0f / 3.0f));
    band_mapper(band_b, rgb_band_b_walk, rgb_band_b_walk + (1.0f / 3.0f));

    Leds &leds(Leds::instance());
    for (size_t c = 0; c < Leds::circleLedsN; c++) {
        auto out = color::srgb({band_r[c], band_g[c], band_b[c]});
        leds.setCircle(0, c, out);
        leds.setCircle(1, Leds::circleLedsN-1-c, out);
    }

    rgb_band_r_walk -= rgb_band_r_walk_step;
    rgb_band_g_walk += rgb_band_g_walk_step;
    rgb_band_b_walk += rgb_band_b_walk_step;
}

void Effects::direction() {
    standard_bird();

    MMC5633NJL::instance().update();

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    vector::float4 col(gradient_rainbow.repeat((MMC5633NJL::instance().Z()+400.0f) * (1.0f/700.0f)));
    calc([=](const vector::float4 &pos) {
        return col;
    });

}

void Effects::static_color() {
    standard_bird();

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([=](const vector::float4 &) {
        return vector::float4(color::srgb8(Model::instance().RingColor()));
    });
}

void Effects::rgb_glow() {
    standard_bird();

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    double now = Timeline::SystemTime();
    const double speed = 0.5;
    float rgb_walk = (static_cast<float>(fmodf(static_cast<float>(now * (1.0 / 5.0) * speed), 1.0)));

    vector::float4 out = color::hsv({rgb_walk, 1.0f, 1.0f});        
    calc([=](const vector::float4 &) {
        return out;
    });
}

void Effects::lightning() {
    standard_bird();

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([=](const vector::float4 &) {
        return color::srgb({0,0,0,0});
    });

    Leds &leds(Leds::instance());
    size_t index = static_cast<size_t>(random.get(static_cast<int32_t>(0),static_cast<int32_t>(16*2 * 16)));
    if (index < Leds::circleLedsN) {
        leds.setCircle(0, index, color::srgb(vector::float4::one()));
    } else if (index < Leds::circleLedsN * 2) {
        leds.setCircle(1, index - Leds::circleLedsN, color::srgb(vector::float4::one()));
    }
}

void Effects::lightning_crazy() {
    standard_bird();

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([=](const vector::float4 &) {
        return color::srgb({0,0,0,0});
    });

    Leds &leds(Leds::instance());
    size_t index = static_cast<size_t>(random.get(static_cast<int32_t>(0),static_cast<int32_t>(Leds::circleLedsN * 2 - 1)));
    if (index < Leds::circleLedsN) {
        leds.setCircle(0, index, color::srgb(vector::float4::one()));
    } else if (index < Leds::circleLedsN * 2) {
        leds.setCircle(1, index - Leds::circleLedsN, color::srgb(vector::float4::one()));
    }
}

void Effects::sparkle() {

    standard_bird();

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([=](const vector::float4 &) {
        return vector::float4(0,0,0,0);
    });

    Leds &leds(Leds::instance());
    size_t index = static_cast<size_t>(random.get(static_cast<int32_t>(0),static_cast<int32_t>(Leds::circleLedsN * 2 - 1)));
    vector::float4 col = vector::float4(
        color::srgb({random.get(0.0f,1.0f),
                     random.get(0.0f,1.0f),
                     random.get(0.0f,1.0f)})
    );
    if (index < Leds::circleLedsN) {
        leds.setCircle(0, index, col);
    } else if (index < Leds::circleLedsN * 2) {
        leds.setCircle(1, index - Leds::circleLedsN, col);
    }

}

void Effects::rando() {
    standard_bird();

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([this](const vector::float4 &) {
        return vector::float4(
            color::srgb({random.get(0.0f,1.0f),
                         random.get(0.0f,1.0f),
                         random.get(0.0f,1.0f)})
        );
    });
}

void Effects::red_green() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([=](const vector::float4 &pos) {
        return color::srgb({pos.x * sinf(now), pos.y * cosf(now), 0, 0});
    });
}

void Effects::brilliance() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    static float next = -1.0f;
    static float dir = 0.0f;
    
    if ((next - now) < 0.0f || next < 0.0f) {
        next = now + random.get(2.0f, 20.0f);
        dir = random.get(0.0f, 3.141f * 2.0f);
    }

    static color::gradient bw({
        color::srgb8_stop(0x000000, 0.00f),
        color::srgb8_stop(0x000000, 1.00f)
    });

    static bool bw_init = false;
    if (!bw_init) {
        bw_init = true;
        bw = color::gradient({
            color::srgb8_stop(Model::instance().RingColor(), 0.00f),
            color::srgb8_stop(Model::instance().RingColor(), 0.14f),
            color::srgb8_stop(0xffffff, 0.21f),
            color::srgb8_stop(Model::instance().RingColor(), 0.28f),
            color::srgb8_stop(Model::instance().RingColor(), 1.00f)
        });
    }

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([=](const vector::float4 &pos) {
        auto p = pos.rotate2d(dir);
        p *= 0.50f;
        p += (next - now) * 8.0f;
        p *= 0.05f;
        return bw.clamp(p.x);
    });
}

void Effects::highlight() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    static float next = -1.0f;
    static float dir = 0.0f;
    
    if ((next - now) < 0.0f || next < 0.0f) {
        next = now + random.get(2.0f, 10.0f);
        dir = random.get(0.0f, 3.141f * 2.0f);
    }

    static color::gradient bw({
        color::srgb8_stop(0x000000, 0.00f),
        color::srgb8_stop(0x000000, 1.00f)
    });
    static bool bw_init = false;
    if (!bw_init) {
        bw_init = true;
        bw = color::gradient({
            color::srgb8_stop(Model::instance().RingColor(), 0.00f),
            color::srgb8_stop(Model::instance().RingColor(), 0.45f),
            color::srgb8_stop(0xffffff, 0.50f),
            color::srgb8_stop(Model::instance().RingColor(), 0.55f),
            color::srgb8_stop(Model::instance().RingColor(), 1.00f)
        });
    }

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([=](const vector::float4 &pos) {
        auto p = pos.rotate2d(dir);
        p *= 0.50f;
        p += (next - now);
        p *= 0.50f;
        return bw.clamp(p.x);
    });

}

void Effects::autumn() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    static constexpr color::gradient g({
        color::srgb8_stop(0x968b3f, 0.00f),
        color::srgb8_stop(0x097916, 0.20f),
        color::srgb8_stop(0x00d4ff, 0.40f),
        color::srgb8_stop(0xffffff, 0.50f),
        color::srgb8_stop(0x8a0e45, 0.80f),
        color::srgb8_stop(0x968b3f, 1.00f)
    });

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([=](const vector::float4 &pos) {
        auto p = pos + 0.5f;
        p = p.rotate2d(now);
        p *= 0.5f;
        p += 1.0f;
        return g.repeat(p.x);
    });

}


void Effects::heartbeat() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    static color::gradient g({
        color::srgb8_stop(0x000000, 0.00f),
        color::srgb8_stop(0x000000, 1.00f)
    });

    static bool g_init = false;
    if (!g_init) {
        g_init = true;
        g = color::gradient({
            color::srgb8_stop(0x000000, 0.00f),
            color::srgb8_stop(Model::instance().RingColor(), 1.00)});
    }

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([=](const vector::float4 &pos) {
        return g.reflect(now);
    });
}

void Effects::moving_rainbow() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([=](const vector::float4 &pos) {
        auto p = pos.rotate2d(-now * 0.25f);
        p += now;
        p *= 0.25f;
        p = p.reflect();
        return color::hsv({p.x, 1.0, 1.0f});
    });
}



void Effects::twinkle() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    static color::gradient g({
        color::srgb8_stop(0x000000, 0.00f),
        color::srgb8_stop(0x000000, 1.00f)
    });
    
    static bool g_init = false;
    static color::rgba<uint8_t> ringColor;

    static constexpr size_t many = 8;
    static float next[many] = { -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f };
    static size_t which[many] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    
    for (size_t c = 0; c < many; c++) {
        if ((next[c] - now) < 0.0f || next[c] < 0.0f) {
            next[c] = now + random.get(0.5f, 4.0f);
            which[c] = static_cast<size_t>(random.get(static_cast<int32_t>(0), Leds::circleLedsN));
        }
    }


    if (!g_init || ringColor != Model::instance().RingColor()) {
        ringColor = Model::instance().RingColor();
        g_init = true;
        g = color::gradient({
            color::srgb8_stop(0x000000, 0.00f),
            color::srgb8_stop(Model::instance().RingColor(), 0.25f),
            color::srgb8_stop(0xFFFFFF, 0.50f),
            color::srgb8_stop(Model::instance().RingColor(), 0.75f),
            color::srgb8_stop(0x000000, 1.00f),
        });
    }

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos, size_t index)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos, c);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([=](const vector::float4 &pos, size_t index) {
        for (size_t c = 0; c < many; c++) {
            if (which[c] == index) {
                return (g.clamp(next[c] - now));
            }
        }
        return vector::float4();
    });
}

void Effects::twinkly() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    static constexpr size_t many = 8;
    static float next[many] = { -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f };
    static size_t which[many] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    
    static color::gradient g({
        color::srgb8_stop(0x000000, 0.00f),
        color::srgb8_stop(0x000000, 1.00f)
    });

    static bool g_init = false;
    if (!g_init) {
        g_init = true;
        g = color::gradient({
            color::srgb8_stop(0x000000, 0.00f),
            color::srgb8_stop(0xFFFFFF, 0.80f),
            color::srgb8_stop(0x000000, 1.00f)
        });
    }

    for (size_t c = 0; c < many; c++) {
        if ((next[c] - now) < 0.0f || next[c] < 0.0f) {
            next[c] = now + random.get(0.5f, 4.0f);
            which[c] = static_cast<size_t>(random.get(static_cast<int32_t>(0), Leds::circleLedsN));
        }
    }

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos, size_t index)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos, c);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    vector::float4 ring(color::srgb8(Model::instance().RingColor()));

    calc([=](const vector::float4 &pos, size_t index) {
        for (size_t c = 0; c < many; c++) {
            if (which[c] == index) {
                return g.clamp(next[c] - now) + ring;
            }
        }
        return ring;
    });
}

void Effects::randomfader() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    static float next = -1.0f;
    static size_t which = 0;
    static vector::float4 color;
    static vector::float4 prev_color;
    
    if ((next - now) < 0.0f || next < 0.0f) {
        next = now + 2.0f;
        which = static_cast<size_t>(random.get(static_cast<int32_t>(0), Leds::circleLedsN));
        prev_color = color;
        color = color::srgb({
            random.get(0.0f,1.0f),
            random.get(0.0f,1.0f),
            random.get(0.0f,1.0f)});
    }

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([=](const vector::float4 &pos) {
        float dist = pos.dist(Leds::instance().map.getCircle(which)) * (next - now);
        if (dist > 1.0f) dist = 1.0f;
        return vector::float4::lerp(color, prev_color, dist);
    });
}

void Effects::brightchaser() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    static color::gradient g({
        color::srgb8_stop(0x000000, 0.00f),
        color::srgb8_stop(0x000000, 1.00f)
    });

    static bool g_init = false;
    static color::rgba<uint8_t> ringColor;
    if (!g_init || ringColor != Model::instance().RingColor()) {
        ringColor = Model::instance().RingColor();
        g_init = true;
        g = color::gradient({
            color::srgb8_stop(0x000000, 0.00f),
            color::srgb8_stop(ringColor, 0.50f),
            color::srgb8_stop(0xFFFFFF, 1.00f)
        });
    }

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([=](const vector::float4 &pos) {
        auto p = pos.rotate2d(now);
        return g.clamp(p.x);
    });
}

void Effects::chaser() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    vector::float4 ring(color::srgb8(Model::instance().RingColor()));

    calc([=](const vector::float4 &pos) {
        auto p = pos.rotate2d(now);
        return ring * p.x;
    });
}

void Effects::gradient() {
    standard_bird();

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    vector::float4 ring(color::srgb8(Model::instance().RingColor()));

    calc([=](const vector::float4 &pos) {
        return (ring * (1.0f - ( (pos.y + 1.0f) * 0.33f ) )).clamp();
    });
}

void Effects::overdrive() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    static color::gradient g({
        color::srgb8_stop(0x000000, 0.00f),
        color::srgb8_stop(0x000000, 1.00f)
    });

    static bool g_init = false;
    static color::rgba<uint8_t> ringColor;
    if (!g_init || ringColor != Model::instance().RingColor()) {
        ringColor = Model::instance().RingColor();
        g_init = true;
        g = color::gradient({
            color::srgb8_stop(0x000000, 0.00f),
            color::srgb8_stop(ringColor, 1.00f)
        });
    }

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
        for (size_t c = 0; c < Leds::birdLedsN; c++) {
            auto pos = Leds::instance().map.getBird(c);
            auto col = func(pos);
            leds.setBird(0, c, col);
        }
    };

    calc([=](const vector::float4 &pos) {
        float x = sinf(pos.x + 1.0f + now * 1.77f);
        float y = cosf(pos.y + 1.0f + now * 2.01f);
        return (g.reflect(x * y) * 8.0f).clamp();
    });
}

void Effects::ironman() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    static color::gradient g({
        color::srgb8_stop(0x000000, 0.00f),
        color::srgb8_stop(0x000000, 1.00f)
    });

    static bool g_init = false;
    static color::rgba<uint8_t> ringColor;
    if (!g_init || ringColor != Model::instance().RingColor()) {
        ringColor = Model::instance().RingColor();
        g_init = true;
        g = color::gradient({
            color::srgb8_stop(0xffffff, 0.00f),
            color::srgb8_stop(ringColor, 0.10f),
            color::srgb8_stop(0x000000, 1.00f)
        });
    }

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
        for (size_t c = 0; c < Leds::birdLedsN; c++) {
            auto pos = Leds::instance().map.getBird(c);
            auto col = func(pos);
            leds.setBird(0, c, col);
        }
    };

    calc([=](const vector::float4 &pos) {
        float len = pos.len();
        return g.clamp(1.0f-((len!=0.0f)?1.0f/len:1000.0f)*(fabsf(sinf(now))));
    });
}

void Effects::sweep() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    static color::gradient g({
        color::srgb8_stop(0x000000, 0.00f),
        color::srgb8_stop(0x000000, 1.00f)
    });

    static bool g_init = false;
    static color::rgba<uint8_t> ringColor;
    if (!g_init || ringColor != Model::instance().RingColor()) {
        ringColor = Model::instance().RingColor();
        g_init = true;
        g = color::gradient({
            color::srgb8_stop(0xffffff, 0.00f),
            color::srgb8_stop(ringColor, 0.50f),
            color::srgb8_stop(ringColor, 1.00f),
        });
    }

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([=](const vector::float4 &pos) {
        return g.reflect(pos.rotate2d(-now * 0.5f).y - now * 8.0f);
    });
}

void Effects::sweephighlight() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    static color::gradient g({
        color::srgb8_stop(0x000000, 0.00f),
        color::srgb8_stop(0x000000, 1.00f)
    });

    static bool g_init = false;
    static color::rgba<uint8_t> ringColor;
    if (!g_init || ringColor != Model::instance().RingColor()) {
        ringColor = Model::instance().RingColor();
        g_init = true;
        g = color::gradient({
            color::srgb8_stop(0xffffff, 0.00f),
            color::srgb8_stop(ringColor, 0.50f),
            color::srgb8_stop(ringColor, 0.70f),
            color::srgb8_stop(0xffffff, 1.00f),
        });
    }

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([=](const vector::float4 &pos) {
        return g.reflect(pos.rotate2d(-now * 0.25f).y - now * 2.0f);
    });
}

void Effects::rainbow_circle() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([=](const vector::float4 &pos) {
        return color::hsv({fmodf((atan2f(pos.x, pos.y) + 3.14159f) / (3.14159f * 2.0f) + now * 0.5f, 1.0f), 1.0f, 1.0f});
    });
}

void Effects::rainbow_grow() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([=](const vector::float4 &pos) {
        return color::hsv({fmodf(fabsf(pos.x * 0.25f + signf(pos.x) * now * 0.25f), 1.0f), 1.0f, 1.0f});
    });
}

void Effects::rotor() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    static color::gradient g({
        color::srgb8_stop(0x000000, 0.00f),
        color::srgb8_stop(0x000000, 1.00f)
    });

    static bool g_init = false;
    static color::rgba<uint8_t> ringColor;
    if (!g_init || ringColor != Model::instance().RingColor()) {
        ringColor = Model::instance().RingColor();
        g_init = true;
        g = color::gradient({
            color::srgb8_stop(ringColor, 0.00f),
            color::srgb8_stop(0xffffff, 0.50f),
            color::srgb8_stop(ringColor, 1.00f),
        });
    }

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([=](const vector::float4 &pos) {
        return g.repeat(fmodf((atan2f(pos.x, pos.y) + 3.14159f) / (3.14159f * 2.0f) + now * 0.5f, 1.0f) * 4.0f);
    });
}

void Effects::rotor_sparse() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    static color::gradient g({
        color::srgb8_stop(0x000000, 0.00f),
        color::srgb8_stop(0x000000, 1.00f)
    });

    static bool g_init = false;
    static color::rgba<uint8_t> ringColor;
    if (!g_init || ringColor != Model::instance().RingColor()) {
        ringColor = Model::instance().RingColor();
        g_init = true;
        g = color::gradient({
            color::srgb8_stop(0x000000, 0.00f),
            color::srgb8_stop(ringColor, 0.45f),
            color::srgb8_stop(ringColor, 0.55f),
            color::srgb8_stop(0x000000, 1.00f),
        });
    }

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([=](const vector::float4 &pos) {
        return g.repeat(fmodf((atan2f(pos.x, pos.y) + 3.14159f) / (3.14159f * 2.0f) + now * 0.5f, 1.0f) * 3.0f);
    });
}

void Effects::fullcolor() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    static constexpr color::gradient g({
        color::srgb8_stop(0x000000, 0.00f),
        color::srgb8_stop(0xFFFFFF, 0.50f),
        color::srgb8_stop(0x000000, 1.00f),
    });

    auto calc = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc([=](const vector::float4 &pos) {
        return color::srgb(vector::float4(
            g.repeat(fmodf((atan2f(pos.x, pos.y) + 3.14159f) / (3.14159f * 2.0f) + now * 0.50f, 1.0f)).x,
            g.repeat(fmodf((atan2f(pos.x, pos.y) + 3.14159f) / (3.14159f * 2.0f) + now * 0.75f, 1.0f)).x,
            g.repeat(fmodf((atan2f(pos.x, pos.y) + 3.14159f) / (3.14159f * 2.0f) + now * 0.33f, 1.0f)).x
        ));
    });
}

void Effects::flip_colors() {
    standard_bird();

    float now = float(Timeline::SystemTime());

    vector::float4 bird(color::srgb8(Model::instance().BirdColor()));
    vector::float4 ring(color::srgb8(Model::instance().RingColor()));

    auto calc_inner = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::birdLedsN; c++) {
            auto pos = Leds::instance().map.getBird(c);
            auto col = func(pos);
            leds.setBird(0, c, col);
            leds.setBird(1, Leds::birdLedsN-1-c, col);
        }
    };

    auto calc_outer = [=](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::circleLedsN; c++) {
            auto pos = Leds::instance().map.getCircle(c);
            auto col = func(pos);
            leds.setCircle(0, c, col);
            leds.setCircle(1, Leds::circleLedsN-1-c, col);
        }
    };

    calc_inner([=](const vector::float4 &pos) {
        return vector::float4::lerp(bird, ring, (sinf(now) + 1.0f) * 0.5f);
    });

    calc_outer([=](const vector::float4 &pos) {
        return vector::float4::lerp(ring, bird, (sinf(now) + 1.0f) * 0.5f);
    });
}

void Effects::init() {

    random.set_seed(Seed::instance().seedU32());

    static Timeline::Effect mainEffect;

    if (!Timeline::instance().Scheduled(mainEffect)) {
        mainEffect.time = Timeline::SystemTime();
        mainEffect.duration = std::numeric_limits<double>::infinity();
        mainEffect.calcFunc = [this](Timeline::Span &, Timeline::Span &) {

            static uint32_t current_effect = 0;
            static uint32_t previous_effect = 0;
            static double switch_time = 0;

            if ( current_effect != Model::instance().Effect() ) {
                previous_effect = current_effect;
                current_effect = Model::instance().Effect();
                switch_time = Timeline::SystemTime();
            }

            auto calc_effect = [this] (uint32_t effect) {
                switch (effect) {
                    case 0:
                        black();
                    break;
                    case 1:
                        static_color();
                    break;
                    case 2:
                        rgb_band();
                    break;
                    case 3:
                        color_walker();
                    break;
                    case 4:
                        light_walker();
                    break;
                    case 5:
                        rgb_glow();
                    break;
                    case 6:
                        lightning();
                    break;
                    case 7:
                        lightning_crazy();
                    break;
                    case 8:
                        sparkle();
                    break;
                    case 9:
                        rando();
                    break;
                    case 10:
                        red_green();
                    break;
                    case 11:
                        brilliance();
                    break;
                    case 12:
                        highlight();
                    break;
                    case 13:
                        autumn();
                    break;
                    case 14:
                        heartbeat();
                    break;
                    case 15:
                        moving_rainbow();
                    break;
                    case 16:
                        twinkle();
                    break;
                    case 17:
                        twinkly();
                    break;
                    case 18:
                        randomfader();
                    break;
                    case 19:
                        chaser();
                    break;
                    case 20:
                        brightchaser();
                    break;
                    case 21:
                        gradient();
                    break;
                    case 22:
                        overdrive();
                    break;
                    case 23:
                        ironman();
                    break;
                    case 24:
                        sweep();
                    break;
                    case 25:
                        sweephighlight();
                    break;
                    case 26:
                        rainbow_circle();
                    break;
                    case 27:
                        rainbow_grow();
                    break;
                    case 28:
                        rotor();
                    break;
                    case 29:
                        rotor_sparse();
                    break;
                    case 30:
                        fullcolor();
                    break;
                    case 31:
                        flip_colors();
                    break;
                    case 32:
                        direction();
                    break;
                }
            };

            double blend_duration = 0.5;
            double now = Timeline::SystemTime();

            Leds &leds(Leds::instance());

            if ((now - switch_time) < blend_duration) {
                calc_effect(previous_effect);

                auto circleLedsPrev = leds.getCircle();
                auto birdsLedsPrev = leds.getBird();

                calc_effect(current_effect);

                auto circleLedsNext = leds.getCircle();
                auto birdsLedsNext = leds.getBird();

                float blend = static_cast<float>(now - switch_time) * (1.0f / static_cast<float>(blend_duration));

                for (size_t c = 0; c < circleLedsNext.size(); c++) {
                    for (size_t d = 0; d < circleLedsNext[c].size(); d++) {
                        circleLedsNext[c][d] = vector::float4::lerp(circleLedsPrev[c][d], circleLedsNext[c][d], blend);
                    }
                }

                leds.setCircle(circleLedsNext);

                for (size_t c = 0; c < birdsLedsNext.size(); c++) {
                    for (size_t d = 0; d < birdsLedsNext[c].size(); d++) {
                        birdsLedsNext[c][d] = vector::float4::lerp(birdsLedsPrev[c][d], birdsLedsNext[c][d], blend);
                    }
                }

                leds.setBird(birdsLedsNext);

            } else {
                calc_effect(current_effect);
            }

        };
        mainEffect.commitFunc = [this](Timeline::Span &) {
            Leds::instance().apply();
        };
        Timeline::instance().Add(mainEffect);
    }

    printf("Effects initialized.\n");
}
