#ifndef ENGINE_SCENE_TILESPRITEINFO_H
#define ENGINE_SCENE_TILESPRITEINFO_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED

class ISprite;


class TileSpriteInfo {
public:
    ISprite* Sprite;
    int AnimationIndex;
    int FrameIndex;

};

#endif /* ENGINE_SCENE_TILESPRITEINFO_H */
