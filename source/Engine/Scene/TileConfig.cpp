#if INTERFACE
class TileConfig {
public:
    Uint8* Collision;
    Uint8* HasCollision;
    Uint8  Config[5];
    Uint16 IsCeiling = false;
};
#endif
