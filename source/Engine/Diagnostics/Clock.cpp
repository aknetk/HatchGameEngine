#if INTERFACE
#include <Engine/Includes/Standard.h>

class Clock {
public:
};
#endif

#include <Engine/Diagnostics/Log.h>
#include <Engine/Diagnostics/Clock.h>

#ifdef WIN32
    #include <windows.h>

    bool          Win32_PerformanceFrequencyEnabled = false;
    LARGE_INTEGER Win32_Frequency;
    double        Win32_CPUFreq;
    Sint64        Win32_GameStartTime;
    stack<double> Win32_ClockStack;
#endif

#include <stack>
#include <ratio>
#include <chrono>
#include <thread>

chrono::steady_clock::time_point        GameStartTime;
stack<chrono::steady_clock::time_point> ClockStack;

PUBLIC STATIC void   Clock::Init() {
    #ifdef WIN32
        if (QueryPerformanceFrequency(&Win32_Frequency)) {
            Win32_PerformanceFrequencyEnabled = true;
            Win32_CPUFreq = (double)Win32_Frequency.QuadPart / 1000.0;
            Log::Print(Log::LOG_INFO, "CPU Frequency: %.1f GHz", Win32_Frequency.QuadPart / 1000000.0);

            LARGE_INTEGER ticks;
            if (QueryPerformanceCounter(&ticks)) {
                Win32_GameStartTime = ticks.QuadPart;
            }
        }
        else {
            Win32_PerformanceFrequencyEnabled = false;
            // Win32_GameStartTime = timeGetTime();
        }
    #endif

    GameStartTime = chrono::steady_clock::now();
}
PUBLIC STATIC void   Clock::Start() {
    #ifdef WIN32
        Win32_ClockStack.push(Clock::GetTicks());
    #endif

    ClockStack.push(chrono::steady_clock::now());
}
PUBLIC STATIC double Clock::GetTicks() {
    #ifdef WIN32
        if (Win32_PerformanceFrequencyEnabled) {
            LARGE_INTEGER ticks;
            if (QueryPerformanceCounter(&ticks)) {
                return (double)(ticks.QuadPart - Win32_GameStartTime) / Win32_CPUFreq;
            }
        }
        else {
            // return (double)(timeGetTime() - Win32_GameStartTime);
        }
    #endif

    return (chrono::steady_clock::now() - GameStartTime).count() / 1000000.0;
}
PUBLIC STATIC double Clock::End() {
    #ifdef WIN32
    {
        auto t1 = Win32_ClockStack.top();
        auto t2 = Clock::GetTicks();
        Win32_ClockStack.pop();
        return t2 - t1;
    }
    #endif

    auto t1 = ClockStack.top();
    auto t2 = chrono::steady_clock::now();
    ClockStack.pop();
    return (t2 - t1).count() / 1000000.0;
}
PUBLIC STATIC void   Clock::Delay(double milliseconds) {
    #ifdef WIN32
        Sleep((DWORD)milliseconds);
        return;
    #endif
    this_thread::sleep_for(chrono::nanoseconds((int)(milliseconds * 1000000.0)));
}
