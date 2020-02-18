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

chrono::steady_clock::time_point        Clock::StartTime;
chrono::steady_clock::time_point        Clock::GameStartTime;
stack<chrono::steady_clock::time_point> Clock::ClockStack;

PUBLIC STATIC void   Clock::Init() {
    Clock::GameStartTime = chrono::steady_clock::now();
}
PUBLIC STATIC void   Clock::Start() {
    ClockStack.push(chrono::steady_clock::now());
}
PUBLIC STATIC double Clock::GetTicks() {
    auto t2 = chrono::steady_clock::now();
    chrono::duration<double, milli> time_span = t2 - Clock::GameStartTime;
    return time_span.count();
}
PUBLIC STATIC double Clock::End() {
    auto t1 = ClockStack.top();
    ClockStack.pop();
    auto t2 = chrono::steady_clock::now();
    chrono::duration<double, milli> time_span = t2 - t1;
    return time_span.count();
}
PUBLIC STATIC void   Clock::Delay(double milliseconds) {
    this_thread::sleep_for(chrono::nanoseconds((int)(milliseconds * 1000000.0)));
}
