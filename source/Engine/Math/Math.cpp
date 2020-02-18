#if INTERFACE
#include <Engine/Includes/Standard.h>
class Math {
public:
};
#endif

#include <Engine/Math/Math.h>
#include <time.h>

PUBLIC STATIC void Math::Init() {
    srand(time(NULL));
}

// Trig functions
PUBLIC STATIC float Math::Cos(float n) {
    return std::cos(n);
}
PUBLIC STATIC float Math::Sin(float n) {
    return std::sin(n);
}
PUBLIC STATIC float Math::Atan(float x, float y) {
    if (x == 0.0f && y == 0.0f) return 0.0f;

    return std::atan2(y, x);
}
PUBLIC STATIC float Math::Distance(float x1, float y1, float x2, float y2) {
    x2 -= x1; x2 *= x2;
    y2 -= y1; y2 *= y2;
    return (float)sqrt(x2 + y2);
}
PUBLIC STATIC float Math::Hypot(float a, float b, float c) {
    return (float)sqrt(a * a + b * b + c * c);
}
// Deterministic functions
PUBLIC STATIC float Math::Abs(float n) {
    return std::abs(n);
}
PUBLIC STATIC float Math::Max(float a, float b) {
    return a > b ? a : b;
}
PUBLIC STATIC float Math::Min(float a, float b) {
    return a < b ? a : b;
}
PUBLIC STATIC float Math::Clamp(float v, float a, float b) {
    return Math::Max(a, Math::Min(v, b));
}
PUBLIC STATIC float Math::Sign(float a) {
    return a < 0.0f ? -1.0f : a > 0.0f ? 1.0f : 0.0f;
}

// Random functions (non-inclusive)
PUBLIC STATIC float Math::Random() {
    return rand() / (float)RAND_MAX;
}
PUBLIC STATIC float Math::RandomMax(float max) {
    return Math::Random() * max;
}
PUBLIC STATIC float Math::RandomRange(float min, float max) {
    return (Math::Random() * (max - min)) + min;
}
