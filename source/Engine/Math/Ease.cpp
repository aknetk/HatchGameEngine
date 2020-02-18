#if INTERFACE
#include <Engine/Includes/Standard.h>
class Ease {
public:

};
#endif

#include <Engine/Math/Ease.h>

#include <Engine/Math/Math.h>

#ifndef PI
#define PI 3.1415926545
#endif

PUBLIC STATIC float Ease::InSine(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
	return sin(1.5707963 * t);
}
PUBLIC STATIC float Ease::OutSine(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
	t -= 1.0;
	return 1 + sin(1.5707963 * t);
}
PUBLIC STATIC float Ease::InOutSine(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
	return 0.5 * (1 + sin(3.1415926 * (t - 0.5)));
}
PUBLIC STATIC float Ease::InQuad(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    return t * t;
}
PUBLIC STATIC float Ease::OutQuad(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    return t * (2 - t);
}
PUBLIC STATIC float Ease::InOutQuad(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    return t < 0.5 ? 2 * t * t : t * (4 - 2 * t) - 1;
}
PUBLIC STATIC float Ease::InCubic(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    return t * t * t;
}
PUBLIC STATIC float Ease::OutCubic(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
	t -= 1.0;
    return 1 + t * t * t;
}
PUBLIC STATIC float Ease::InOutCubic(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
	if (t >= 0.5)
		t -= 1.0;
    return t < 0.5 ? 4 * t * t * t : 1 + t * (2 * (t - 1)) * (2 * (t - 2));
}
PUBLIC STATIC float Ease::InQuart(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    t *= t;
    return t * t;
}
PUBLIC STATIC float Ease::OutQuart(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    t = (t - 1) * (t - 1);
    return 1 - t * t;
}
PUBLIC STATIC float Ease::InOutQuart(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    if (t < 0.5) {
        t *= t;
        return 8 * t * t;
    }
    else {
        t = (t - 1) * (t - 1);
        return 1 - 8 * t * t;
    }
}
PUBLIC STATIC float Ease::InQuint(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    float t2 = t * t;
    return t * t2 * t2;
}
PUBLIC STATIC float Ease::OutQuint(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    float t2 = (t - 1) * (t - 1);
    return 1 + t * t2 * t2;
}
PUBLIC STATIC float Ease::InOutQuint(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    float t2;
    if (t < 0.5) {
        t2 = t * t;
        return 16.0f * t * t2 * t2;
    }
    else {
		t = 1.0f - t;
		t2 = t * t;
        return 1.0f - 16.0f * t * t2 * t2;
    }
}
PUBLIC STATIC float Ease::InExpo(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    return (pow(2, 8 * t) - 1) / 255;
}
PUBLIC STATIC float Ease::OutExpo(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    return 1 - pow(2, -8 * t);
}
PUBLIC STATIC float Ease::InOutExpo(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    if (t < 0.5) {
        return (pow(2, 16 * t) - 1) / 510;
    }
    else {
        return 1 - 0.5 * pow(2, -16 * (t - 0.5));
    }
}
PUBLIC STATIC float Ease::InCirc(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    return 1 - sqrt(1 - t);
}
PUBLIC STATIC float Ease::OutCirc(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    return sqrt(t);
}
PUBLIC STATIC float Ease::InOutCirc(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    if (t < 0.5) {
        return (1 - sqrt(1 - 2 * t)) * 0.5;
    }
    else {
        return (1 + sqrt(2 * t - 1)) * 0.5;
    }
}
PUBLIC STATIC float Ease::InBack(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    return t * t * (2.70158 * t - 1.70158);
}
PUBLIC STATIC float Ease::OutBack(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
	t -= 1.0;
    return 1 + t * t * (2.70158 * t + 1.70158);
}
PUBLIC STATIC float Ease::InOutBack(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    if (t < 0.5) {
        return t * t * (7 * t - 2.5) * 2;
    }
    else {
		t -= 1.0;
        return 1 + t * t * 2 * (7 * t + 2.5);
    }
}
PUBLIC STATIC float Ease::InElastic(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    float t2 = t * t;
    return t2 * t2 * sin(t * PI * 4.5);
}
PUBLIC STATIC float Ease::OutElastic(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    float t2 = (t - 1) * (t - 1);
    return 1 - t2 * t2 * cos(t * PI * 4.5);
}
PUBLIC STATIC float Ease::InOutElastic(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    float t2;
    if (t < 0.45) {
        t2 = t * t;
        return 8 * t2 * t2 * sin(t * PI * 9);
    }
    else if (t < 0.55) {
        return 0.5 + 0.75 * sin(t * PI * 4);
    }
    else {
        t2 = (t - 1) * (t - 1);
        return 1 - 8 * t2 * t2 * sin(t * PI * 9);
    }
}
PUBLIC STATIC float Ease::InBounce(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    return pow(2, 6 * (t - 1)) * abs(sin(t * PI * 3.5));
}
PUBLIC STATIC float Ease::OutBounce(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    return 1 - pow(2, -6 * t) * abs(cos(t * PI * 3.5));
}
PUBLIC STATIC float Ease::InOutBounce(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    if (t < 0.5) {
        return 8 * pow(2, 8 * (t - 1)) * abs(sin(t * PI * 7));
    }
    else {
        return 1 - 8 * pow(2, -8 * t) * abs(sin(t * PI * 7));
    }
}
PUBLIC STATIC float Ease::Triangle(float t) {
	t = Math::Clamp(t, 0.0f, 1.0f);
    if (t < 0.5) {
        return t * 2.0;
    }
    else {
        return 2.0 - t * 2.0;
    }
}
