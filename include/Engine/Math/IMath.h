#ifndef ENGINE_MATH_IMATH_H
#define ENGINE_MATH_IMATH_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED


#include <Engine/Includes/Standard.h>

class Math {
public:
    static int CosValues[256];
    static int SinValues[256];
    static int AtanValues[258];
    static double CosDoubleValues[256];
    static double SinDoubleValues[256];
    static char Str[258];

    static void Init();
    static int Cos(int n);
    static int Sin(int n);
    static int Atan(int x, int y);
    static int Distance(int x1, int y1, int x2, int y2);
    static double CosDouble(int n);
    static double SinDouble(int n);
    static int Abs(int n);
    static int Max(int a, int b);
    static int Min(int a, int b);
    static int Clamp(int v, int a, int b);
    static int Sign(int a);
    static int Random();
    static int RandomMax(int max);
    static int RandomRange(int min, int max);
    static char* ToString(int a);
};

#endif /* ENGINE_MATH_IMATH_H */
