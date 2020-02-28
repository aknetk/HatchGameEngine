#if INTERFACE
#include <Engine/Includes/Standard.h>

#include <stack>
#include <ratio>
#include <chrono>
#include <thread>

class Clock {
private:
    static chrono::steady_clock::time_point         StartTime;
    static chrono::steady_clock::time_point         GameStartTime;
    static stack<chrono::steady_clock::time_point>  ClockStack;
};
#endif

#include <Engine/Diagnostics/Log.h>
#include <Engine/Diagnostics/Clock.h>

#ifdef WIN32
#include <windows.h>

LARGE_INTEGER frequency;
stack<double> clockStack;
#endif

chrono::steady_clock::time_point        Clock::StartTime;
chrono::steady_clock::time_point        Clock::GameStartTime;
stack<chrono::steady_clock::time_point> Clock::ClockStack;

PUBLIC STATIC void   Clock::Init() {
    #ifdef WIN32
    if (QueryPerformanceFrequency(&frequency)) {
        // return (double)ticks.QuadPart / 1000.0;
    }
    #endif

    Clock::GameStartTime = chrono::steady_clock::now();
}
PUBLIC STATIC void   Clock::Start() {
    #ifdef WIN32
    clockStack.push(Clock::GetTicks());
    #endif

    ClockStack.push(chrono::steady_clock::now());
}
PUBLIC STATIC double Clock::GetTicks() {
    #ifdef WIN32
    LARGE_INTEGER ticks;
    if (QueryPerformanceCounter(&ticks)) {
        return (double)ticks.QuadPart * 1000.0 / frequency.QuadPart;
    }
    #endif

    auto t2 = chrono::steady_clock::now();
    chrono::duration<double, milli> time_span = t2 - Clock::GameStartTime;
    return time_span.count();
}
PUBLIC STATIC double Clock::End() {
    #ifdef WIN32
    auto t1_ = clockStack.top();
    ClockStack.pop();
    auto t2_ = Clock::GetTicks();
    return t2_ - t1_;
    #endif

    auto t1 = ClockStack.top();
    ClockStack.pop();
    auto t2 = chrono::steady_clock::now();
    chrono::duration<double, milli> time_span = t2 - t1;
    return time_span.count();
}
PUBLIC STATIC void   Clock::Delay(double milliseconds) {
    this_thread::sleep_for(chrono::nanoseconds((int)(milliseconds * 1000000.0)));
}
