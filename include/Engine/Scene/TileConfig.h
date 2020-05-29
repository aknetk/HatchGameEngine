#ifndef ENGINE_SCENE_TILECONFIG_H
#define ENGINE_SCENE_TILECONFIG_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED



class TileConfig {
public:
    Uint8* Collision;
    Uint8* HasCollision;
    Uint8  Config[5];
    Uint16 IsCeiling = false;

};

#endif /* ENGINE_SCENE_TILECONFIG_H */
