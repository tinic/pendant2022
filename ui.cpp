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
#include "./ui.h"
#include "./timeline.h"
#include "./sdd1306.h"
#include "./ens210.h"
#include "./bq25895.h"
#include "./model.h"
#include "./leds.h"

#include <stdio.h>

UI &UI::instance() {
    static UI ui;
    if (!ui.initialized) {
        ui.initialized = true;
        ui.init();
    }
    return ui;
}

void UI::FlipAnimation(Timeline::Span *parent) {
	static Timeline::Display flipSpan;

	if (Timeline::instance().Scheduled(flipSpan)) {
		return;
	}

	flipSpan.time = Timeline::SystemTime();
	flipSpan.duration = 0.25; // timeout
	flipSpan.startFunc = [=](Timeline::Span &) {
		SDD1306::instance().SetVerticalShift(0);
		SDD1306::instance().SetBootScreen(false, 0);
		SDD1306::instance().Display();
	};
	flipSpan.calcFunc = [=](Timeline::Span &span, Timeline::Span &below) {
		double now = Timeline::SystemTime();
		float delta = 1.0f - static_cast<float>( ( (span.time + span.duration) - now ) / span.duration );
		if (delta > 0.5f) {
			below.Calc();
			delta = 1.0f - (2.0f * (delta - 0.5f));
			delta = Cubic::easeOut(delta, 0.0f, 1.0f, 1.0f);
			SDD1306::instance().SetCenterFlip(static_cast<int8_t>(48.0f * delta));
		} else {
			parent->Calc();
			delta = 1.0f - (2.0f * (0.5f - delta));
			delta = Cubic::easeIn(delta, 0.0f, 1.0f, 1.0f);
			SDD1306::instance().SetCenterFlip(static_cast<int8_t>(48.0f * delta));
		}
	};
	flipSpan.commitFunc = [=](Timeline::Span &) {
		SDD1306::instance().Display();
	};
	flipSpan.doneFunc = [=](Timeline::Span &span) {
		SDD1306::instance().SetCenterFlip(0);
		SDD1306::instance().Display();
		Timeline::instance().Remove(span);
	};
    Timeline::instance().Add(flipSpan);
}

void UI::enterColorPrefs(Timeline::Span &) {
    static Timeline::Display prefsDisplay;

    static uint8_t duckOrBird = 0;
    static uint8_t rgbSelection = 0;

    static Timeline::Effect colorEffect;
    colorEffect.time = Timeline::SystemTime();
    colorEffect.duration = std::numeric_limits<double>::infinity();
    colorEffect.calcFunc = [this](Timeline::Span &, Timeline::Span &) {
        vector::float4 bird(color::srgb8(Model::instance().BirdColor()));
        {
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
        {
            vector::float4 circle(color::srgb8(Model::instance().RingColor()));

            auto calc = [](const std::function<vector::float4 ()> &func) {
                Leds &leds(Leds::instance());
                for (size_t c = 0; c < Leds::circleLedsN; c++) {
                    auto col = func();
                    leds.setCircle(0,c,col);
                    leds.setCircle(1,c,col);
                }
            };

            calc([=]() {
                return circle;
            });
        }
    };
    colorEffect.commitFunc = [this](Timeline::Span &) {
        Leds::instance().apply();
    };


    prefsDisplay.time = Timeline::SystemTime();
    prefsDisplay.duration = 10.0; // timeout
    prefsDisplay.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
        if (duckOrBird == 0) {
            SDD1306::instance().PlaceCustomChar(0,0,0xA0);
            SDD1306::instance().PlaceCustomChar(1,0,0xA1);
            SDD1306::instance().PlaceCustomChar(2,0,0xA2);
            SDD1306::instance().PlaceCustomChar(3,0,0xA3);
            SDD1306::instance().PlaceCustomChar(4,0,0xA4);
            SDD1306::instance().PlaceCustomChar(5,0,0xA5);
            SDD1306::instance().PlaceCustomChar(6,0,0xA6);
            SDD1306::instance().PlaceCustomChar(7,0,0xA7);
        } else {
            SDD1306::instance().PlaceCustomChar(0,0,0xA8);
            SDD1306::instance().PlaceCustomChar(1,0,0xA9);
            SDD1306::instance().PlaceCustomChar(2,0,0xAA);
            SDD1306::instance().PlaceCustomChar(3,0,0xAB);
            SDD1306::instance().PlaceCustomChar(4,0,0xAC);
            SDD1306::instance().PlaceCustomChar(5,0,0xAD);
            SDD1306::instance().PlaceCustomChar(6,0,0xAE);
            SDD1306::instance().PlaceCustomChar(7,0,0xAF);
        }

        switch (rgbSelection) {
            case 0: {
                SDD1306::instance().PlaceCustomChar(0,1,0x98);
                SDD1306::instance().PlaceCustomChar(0,2,0x9B);
                SDD1306::instance().PlaceCustomChar(0,3,0x9D);
            } break;
            case 1: {
                SDD1306::instance().PlaceCustomChar(0,1,0x99);
                SDD1306::instance().PlaceCustomChar(0,2,0x9A);
                SDD1306::instance().PlaceCustomChar(0,3,0x9D);
            } break;
            case 2: {
                SDD1306::instance().PlaceCustomChar(0,1,0x99);
                SDD1306::instance().PlaceCustomChar(0,2,0x9B);
                SDD1306::instance().PlaceCustomChar(0,3,0x9C);
            } break;
        }


        color::rgba<uint8_t> col;
        if (duckOrBird == 0) {
            col = Model::instance().BirdColor();
        } else {
            col = Model::instance().RingColor();
        }

        SDD1306::instance().PlaceBar(1,1,7,uint8_t(uint32_t((col.r)*13)/255),1);
        SDD1306::instance().PlaceBar(1,2,7,uint8_t(uint32_t((col.g)*13)/255),1);
        SDD1306::instance().PlaceBar(1,3,7,uint8_t(uint32_t((col.b)*13)/255),1);
    };

    prefsDisplay.commitFunc = [=](Timeline::Span &) {
        SDD1306::instance().Display();
    };

    prefsDisplay.doneFunc = [this](Timeline::Span &) {
		FlipAnimation(&prefsDisplay);
        Timeline::instance().Remove(colorEffect);
    };

    prefsDisplay.switch1Func = [=](Timeline::Span &, bool up) {
        if (up) {
            prefsDisplay.time = Timeline::SystemTime(); // reset timeout
            rgbSelection ++;
            rgbSelection %= 3;
        }
    };

    prefsDisplay.switch2Func = [=](Timeline::Span &, bool up) {
        if (up) {
            prefsDisplay.time = Timeline::SystemTime(); // reset timeout
            duckOrBird ++;
            duckOrBird %= 2;
        }
    };

    prefsDisplay.switch3Func = [=](Timeline::Span &, bool up) {
        if (up) {
            prefsDisplay.time = Timeline::SystemTime(); // reset timeout

            color::rgba<uint8_t> col;
            if (duckOrBird == 0) {
                col = Model::instance().BirdColor();
            } else {
                col = Model::instance().RingColor();
            }

            switch (rgbSelection) {
                case 0: {
                    col.r += 255/14;
                } break;
                case 1: {
                    col.g += 255/14;
                } break;
                case 2: {
                    col.b += 255/14;
                } break;
            }

            if (duckOrBird == 0) {
                Model::instance().SetBirdColor(col);
            } else {
                Model::instance().SetRingColor(col);
            }
        }
    };

    Timeline::instance().Add(prefsDisplay);
    Timeline::instance().Add(colorEffect);
}

void UI::init() {
    static Timeline::Display mainUI;
    if (!Timeline::instance().Scheduled(mainUI)) {
        mainUI.time = Timeline::SystemTime();
        mainUI.duration = std::numeric_limits<double>::infinity();

        mainUI.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
            SDD1306::instance().ClearChar();

            SDD1306::instance().PlaceCustomChar(0,0,0xC0);
            SDD1306::instance().PlaceCustomChar(1,0,0xC1);
            SDD1306::instance().PlaceCustomChar(2,0,0xC2);
            SDD1306::instance().PlaceCustomChar(3,0,0xC3);
            SDD1306::instance().PlaceCustomChar(4,0,0xC4);
            SDD1306::instance().PlaceCustomChar(5,0,0xC5);
            SDD1306::instance().PlaceCustomChar(6,0,0xC6);
            SDD1306::instance().PlaceCustomChar(7,0,0xC7);

            SDD1306::instance().PlaceCustomChar(0,1,0xD0);
            float brightnessLevel = float(Model::instance().BrightnessLevel())/float(Model::instance().BrightnessLevelCount()-1);
            SDD1306::instance().PlaceBar(1,1,7,uint8_t(brightnessLevel*13),1);

            float batteryLevel = ( BQ25895::instance().BatteryVoltage() - BQ25895::instance().MinBatteryVoltage() ) / 
                                 ( BQ25895::instance().MaxBatteryVoltage() - BQ25895::instance().MinBatteryVoltage() );
            batteryLevel = std::max(0.0f, std::min(1.0f, batteryLevel));

            SDD1306::instance().PlaceCustomChar(0,2,0xCF);
            SDD1306::instance().PlaceBar(1,2,7,uint8_t(batteryLevel*13),1);

            SDD1306::instance().PlaceCustomChar(0,3,0xCE);
            char str[32];
            sprintf(str, "[%02d/%02d]", Model::instance().Effect(), Model::instance().EffectCount());
            SDD1306::instance().PlaceUTF8String(1,3,str);
        };
        mainUI.commitFunc = [=](Timeline::Span &) {
            SDD1306::instance().Display();
        };
        mainUI.switch1Func = [=](Timeline::Span &, bool up) {
            if (up) { 
                Model::instance().SetEffect((Model::instance().Effect() + 1) % Model::instance().EffectCount());
            }
        };
        mainUI.switch2Func = [this](Timeline::Span &span, bool up) {
            if (up) { 
                enterColorPrefs(span);
            }
        };
        mainUI.switch3Func = [=](Timeline::Span &, bool up) {
            if (up) { 
                Model::instance().SetBrightnessLevel(Model::instance().BrightnessLevel() + 1);
                if (Model::instance().BrightnessLevel() >= Model::instance().BrightnessLevelCount()) {
                    Model::instance().SetBrightnessLevel(0);
                }
                if (Model::instance().BrightnessLevel() < 3) {
                    SDD1306::instance().DisplayOff();
                } else {
                    SDD1306::instance().DisplayOn();
                }
            }
        };
        Timeline::instance().Add(mainUI);
    }

    static Timeline::Display bootScreen;
    bootScreen.time = Timeline::SystemTime();
    bootScreen.duration = 1.0; // timeout

    static Timeline::Display moveOut;
    moveOut.time = bootScreen.time + bootScreen.duration;
    moveOut.duration = 0.25; // timeout

    static Timeline::Display moveIn;
    moveIn.time = moveOut.time + moveOut.duration;
    moveIn.duration = 0.25; // timeout

    bootScreen.startFunc = [=](Timeline::Span &) {
        SDD1306::instance().ClearChar();
        SDD1306::instance().ClearAttr();
        SDD1306::instance().SetBootScreen(true, 100);
        SDD1306::instance().Display();
    };
    bootScreen.calcFunc = [=](Timeline::Span &span, Timeline::Span &) {
        double now = Timeline::SystemTime();
        double delta = ( (span.time + span.duration) - now ) / span.duration;
        SDD1306::instance().SetBootScreen(true, static_cast<int32_t>(100.0f * Cubic::easeIn(static_cast<float>(delta), 0.0f, 1.0f, 1.0f)));
        SDD1306::instance().Display();
    };
    bootScreen.doneFunc = [=](Timeline::Span &span) {
        SDD1306::instance().SetBootScreen(true, 0);
        SDD1306::instance().Display();
        Timeline::instance().Add(moveOut);
        Timeline::instance().Remove(span);
    };

    moveOut.startFunc = [=](Timeline::Span &) {
    };

    moveOut.calcFunc = [=](Timeline::Span &span, Timeline::Span &) {
        double now = Timeline::SystemTime();
        double delta = ( (span.time + span.duration) - now ) / span.duration;
        SDD1306::instance().SetVerticalShift(-static_cast<int8_t>(16.0f * (1.0f - Cubic::easeOut(static_cast<float>(delta), 0.0f, 1.0f, 1.0f))));
        SDD1306::instance().Display();
    };
    moveOut.doneFunc = [=](Timeline::Span &span) {
        SDD1306::instance().SetVerticalShift(0);
        SDD1306::instance().SetBootScreen(false, 0);
        SDD1306::instance().SetCenterFlip(48);
        SDD1306::instance().Display();
        Timeline::instance().Add(moveIn);
        Timeline::instance().Remove(span);
    };

    moveIn.startFunc = [=](Timeline::Span &) {
        SDD1306::instance().SetVerticalShift(0);
        SDD1306::instance().SetBootScreen(false, 0);
        SDD1306::instance().Invalidate();
        SDD1306::instance().Display();
    };
    moveIn.calcFunc = [=](Timeline::Span &span, Timeline::Span &below) {
        below.Calc();
        double now = Timeline::SystemTime();
        double delta = ( (span.time + span.duration) - now ) / span.duration;
        SDD1306::instance().SetCenterFlip(static_cast<int8_t>(48.0 * (delta)));
        SDD1306::instance().Display();
    };
    moveIn.doneFunc = [=](Timeline::Span &span) {
        SDD1306::instance().SetCenterFlip(0);
        SDD1306::instance().Display();
        Timeline::instance().Remove(span);
    };

    Timeline::instance().Add(bootScreen);

    if (Model::instance().BrightnessLevel() < 3) {
        SDD1306::instance().DisplayOff();
    } else {
        SDD1306::instance().DisplayOn();
    }
}
