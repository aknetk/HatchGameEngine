#ifndef CLOCK_H
#define CLOCK_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


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


public:
    static void   Init();
    static void   Start();
    static double GetTicks();
    static double End();
    static void   Delay(double milliseconds);
};

#endif /* CLOCK_H */
