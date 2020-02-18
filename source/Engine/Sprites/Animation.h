#ifndef ENGINE_SPRITES_ANIMATION_H
#define ENGINE_SPRITES_ANIMATION_H

struct CollisionBox {
    int Left;
    int Top;
    int Right;
    int Bottom;
};
struct AnimFrame {
    int           X;
    int           Y;
    int           Width;
    int           Height;
    int           OffsetX;
    int           OffsetY;
    int           SheetNumber;
    int           Duration;
    int           ID;

    int           BoxCount;
    CollisionBox* Boxes = NULL;
};
struct Animation {
    char*             Name;
    int               AnimationSpeed;
    int               FrameToLoop;
    int               Flags;
    vector<AnimFrame> Frames;
};

#endif /* ENGINE_SPRITES_ANIMATION_H */
