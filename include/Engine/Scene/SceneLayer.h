#ifndef SCENELAYER_H
#define SCENELAYER_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL

class ScrollingInfo;

#include <Engine/Includes/Standard.h>
#include <Engine/Scene/ScrollingInfo.h>
#include <Engine/Scene/ScrollingIndex.h>

class SceneLayer {
public:
    char           Name[50];
    bool           Visible = true;
    int            Width = 0;
    int            Height = 0;
    int            RelativeY = 0x0100;
    int            ConstantY = 0x0000;
    int            OffsetX = 0x0000;
    int            OffsetY = 0x0000;
    bool           UseDeltaCameraX = false;
    bool           UseDeltaCameraY = false;
    Uint32*        Tiles = NULL;
    Uint32*        TilesBackup = NULL;
    Uint16*        TileOffsetY = NULL;
    Sint8*         Deform = NULL;
    int            Flags = 0x0000;
    int            ScrollInfoCount = 0;
    ScrollingInfo* ScrollInfos = NULL;
    Uint8*         ScrollIndexes = NULL;
    Uint32         BufferID = 0;
    int            VertexCount = 0;
    enum {
    FLAGS_COLLIDEABLE = 1,
    FLAGS_NO_REPEAT_X = 2,
    FLAGS_NO_REPEAT_Y = 4,
    }; 

            SceneLayer();
            SceneLayer(int w, int h);
    void    Dispose();
};

#endif /* SCENELAYER_H */
