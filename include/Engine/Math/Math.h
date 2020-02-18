#ifndef MATH_H
#define MATH_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Includes/Standard.h>

class Math {
public:
    static void Init();
    static float Cos(float n);
    static float Sin(float n);
    static float Atan(float x, float y);
    static float Distance(float x1, float y1, float x2, float y2);
    static float Hypot(float a, float b, float c);
    static float Abs(float n);
    static float Max(float a, float b);
    static float Min(float a, float b);
    static float Clamp(float v, float a, float b);
    static float Sign(float a);
    static float Random();
    static float RandomMax(float max);
    static float RandomRange(float min, float max);
};

#endif /* MATH_H */
