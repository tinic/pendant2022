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

#include <stdio.h>

UI &UI::instance() {
    static UI ui;
    if (!ui.initialized) {
        ui.initialized = true;
        ui.init();
    }
    return ui;
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
            sprintf(str, "[%02d|%02d]", Model::instance().Effect(), Model::instance().EffectCount());
            SDD1306::instance().PlaceUTF8String(1,3,str);
        };
        mainUI.commitFunc = [=](Timeline::Span &) {
            SDD1306::instance().Display();
        };
        mainUI.switch1Func = [=](Timeline::Span &, bool up) {
            if (up) { 
                printf("SW1\n");
                Model::instance().SetEffect((Model::instance().Effect() + 1) % Model::instance().EffectCount());
            }
        };
        mainUI.switch2Func = [=](Timeline::Span &, bool up) {
            if (up) { 
                printf("SW2\n");
            }
        };
        mainUI.switch3Func = [=](Timeline::Span &, bool up) {
            if (up) { 
                printf("SW3\n");
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
