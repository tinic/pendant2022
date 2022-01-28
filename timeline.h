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
#ifndef TIMELINE_H_
#define TIMELINE_H_

#include <cstdint>
#include <functional>
#include <tuple>
#include <random>

class Quad {
public:
    static float easeIn(float t, float b, float c, float d);
    static float easeOut(float t, float b, float c, float d);
    static float easeInOut(float t, float b, float c, float d);
};

class Cubic {
public:
    static float easeIn(float t, float b, float c, float d);
    static float easeOut(float t, float b, float c, float d);
    static float easeInOut(float t, float b, float c, float d);
};

class Timeline {
public:

    static constexpr double effectRate = 120.0;
    static constexpr double displayRate = 60.0;
    static constexpr double backgroundRate = 1.0;

    static constexpr double idleRate = 30.0; // once a minute

    struct Span {

        double time = 0.0;
        double duration = 0.0;

        std::function<void (Span &span)> startFunc;
        std::function<void (Span &span, Span &below)> calcFunc;
        std::function<void (Span &span)> commitFunc;
        std::function<void (Span &span)> doneFunc;

        void Start() { if (startFunc) startFunc(*this); }
        void Calc() { if (calcFunc) calcFunc(*this, Timeline::instance().Below(this, type)); }
        void Commit() { if (commitFunc) commitFunc(*this); }
        void Done() { if (doneFunc) doneFunc(*this); }
        
        bool Valid() const { return type != None; }

    protected:

        enum Type {
            None,
            Effect,
            Display,
            Interval,
            Event
        };

        Type type = None;

    private:

        friend class Timeline;
        bool active = false;
        Span *next = 0;
    };

    struct Event : public Span {

        Event() : Span() { type = Type::Event; }

    };

    struct Interval : public Span {

        Interval() : Span() { type = Type::Interval; }

        double interval = 0.0;
        double intervalFuzz = 0.0;

    };

    struct Effect : public Span {

        Effect() : Span() { type = Type::Effect; }

        double attack = 0.0;
        double decay = 0.0;
        double release = 0.0;

        std::tuple<bool, float> InAttackPeriod() const;
        std::tuple<bool, float> InDecayPeriod() const;
        std::tuple<bool, float> InSustainPeriod() const;
        std::tuple<bool, float> InReleasePeriod() const;
    };

    struct Display : public Span {

        Display() : Span() { type = Type::Display; }

        std::function<void (Span &span, bool down)> switch1Func;
        std::function<void (Span &span, bool down)> switch2Func;
        std::function<void (Span &span, bool down)> switch3Func;

        void ProcessSwitch1(bool down) { if (switch1Func) switch1Func(*this, down); }
        void ProcessSwitch2(bool down) { if (switch2Func) switch2Func(*this, down); }
        void ProcessSwitch3(bool down) { if (switch3Func) switch3Func(*this, down); }
    };

    static Timeline &instance();

    bool CheckEffectReadyAndClear();
    bool CheckDisplayReadyAndClear();
    bool CheckBackgroundReadyAndClear();
    bool CheckIdleReadyAndClear();

    void Add(Timeline::Span &span);
    void Remove(Timeline::Span &span);
    bool Scheduled(Timeline::Span &span);

    void ProcessEvent();
    Event &TopEvent() const;

    void ProcessEffect();
    Effect &TopEffect() const;

    void ProcessDisplay();
    Display &TopDisplay() const;

    void ProcessInterval();
    Interval &TopInterval() const;

    static double SystemTime();
    static uint64_t FastSystemTime();
    static uint64_t FastSystemTimeCmp();

private:
    void Process(Span::Type type);
    Span &Top(Span::Type type) const;
    Span &Below(Span *context, Span::Type type) const;

    Span *head = 0;

    void init();
    bool initialized = false;

    std::mt19937 gen;
};

#endif /* TIMELINE_H_ */
