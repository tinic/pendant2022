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
#ifndef EFFECTS_H_
#define EFFECTS_H_

#include <stdint.h>

class Effects {
public:
    static Effects &instance();

private:

    class pseudo_random {
    public:
        
        void set_seed(uint32_t seed) {
            uint32_t i;
            a = 0xf1ea5eed, b = c = d = seed;
            for (i=0; i<20; ++i) {
                (void)get();
            }
        }

        #define rot(x,k) (((x)<<(k))|((x)>>(32-(k))))
        uint32_t get() {
            uint32_t e = a - rot(b, 27);
            a = b ^ rot(c, 17);
            b = c + d;
            c = d + e;
            d = e + a;
            return d;
        }

        float get(float lower, float upper) {
            return static_cast<float>(static_cast<double>(get()) * (static_cast<double>(upper-lower)/static_cast<double>(1LL<<32)) ) + lower;
        }

        int32_t get(int32_t lower, int32_t upper) {
            return (static_cast<int32_t>(get()) % (upper-lower)) + lower;
        }

    private:
        uint32_t a = 0; 
        uint32_t b = 0; 
        uint32_t c = 0; 
        uint32_t d = 0; 

    } random = pseudo_random();

    void standard_bird();

    void color_walker();
    void light_walker();
    void rgb_band();
    void brilliance();
    void direction();
    void black();
    void static_color();
    void rgb_glow();
    void lightning();
    void lightning_crazy();
    void sparkle();
    void rando();
    void red_green();
    void highlight();
    void autumn();
    void heartbeat();
    void moving_rainbow();
    void twinkle();
    void twinkly();
    void randomfader();
    void chaser();
    void brightchaser();
    void gradient();
    void overdrive();
    void ironman();
    void sweep();
    void sweephighlight();
    void rainbow_circle();
    void rainbow_grow();
    void rotor();
    void rotor_sparse();
    void fullcolor();
    void flip_colors();

    void init();
    bool initialized = false;
};

#endif /* EFFECTS_H_ */
