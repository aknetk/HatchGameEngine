#ifndef STANDARDLIBS_H
#define STANDARDLIBS_H

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include <unordered_map>
#include <algorithm>
#include <string>
#include <vector>
#include <deque>
#include <stack>
#include <csetjmp>

#if WIN32
    #define snprintf _snprintf
    #undef __useHeader
    #undef __on_failure
#endif

enum class Platforms {
    Default,
    Windows,
    MacOSX,
    Linux,
    Ubuntu,
    Switch,
    Playstation,
    Xbox,
    Android,
    iOS,
};

#define Uint8 uint8_t
#define Uint16 uint16_t
#define Uint32 uint32_t
#define Uint64 uint64_t
#define Sint8 int8_t
#define Sint16 int16_t
#define Sint32 int32_t
#define Sint64 int64_t

using namespace std;

#endif // STANDARDLIBS_H
