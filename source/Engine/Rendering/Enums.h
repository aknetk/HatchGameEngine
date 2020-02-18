#ifndef ENGINE_RENDERING_ENUMS
#define ENGINE_RENDERING_ENUMS

enum {
    BlendMode_NORMAL = 0,
    BlendMode_ADD = 1,
    BlendMode_MAX = 2,
    BlendMode_SUBTRACT = 3,
};

enum {
    BlendFactor_ZERO = 0,
    BlendFactor_ONE = 1,
    BlendFactor_SRC_COLOR = 2,
    BlendFactor_INV_SRC_COLOR = 3,
    BlendFactor_SRC_ALPHA = 4,
    BlendFactor_INV_SRC_ALPHA = 5,
    BlendFactor_DST_COLOR = 6,
    BlendFactor_INV_DST_COLOR = 7,
    BlendFactor_DST_ALPHA = 8,
    BlendFactor_INV_DST_ALPHA = 9,
};

struct Viewport {
    float X;
    float Y;
    float Width;
    float Height;
};
struct ClipArea {
    bool Enabled;
    float X;
    float Y;
    float Width;
    float Height;
};
struct Point {
    float X;
    float Y;
    float Z;
};

#endif /* ENGINE_RENDERING_ENUMS */
