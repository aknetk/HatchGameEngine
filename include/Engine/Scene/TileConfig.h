#ifndef TILECONFIG_H
#define TILECONFIG_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL



class TileConfig {
public:
    Uint8* Collision;
    Uint8* HasCollision;
    Uint8  Config[5];
    Uint16 IsCeiling = false;

};

#endif /* TILECONFIG_H */
