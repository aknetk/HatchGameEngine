#ifndef ENGINE_SCENE_SCENELAYER_H
#define ENGINE_SCENE_SCENELAYER_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED

class ScrollingInfo;

#include <Engine/Includes/Standard.h>
#include <Engine/Bytecode/Types.h>
#include <Engine/Scene/ScrollingInfo.h>
#include <Engine/Scene/ScrollingIndex.h>

class SceneLayer {
public:
    char           Name[50];
    bool           Visible = true;
    int            Width = 0;
    int            Height = 0;
    Uint32         WidthMask = 0;
    Uint32         HeightMask = 0;
    Uint32         WidthInBits = 0;
    Uint32         HeightInBits = 0;
    Uint32         WidthData = 0;
    Uint32         HeightData = 0;
    Uint32         DataSize = 0;
    Uint32         ScrollIndexCount = 0;
    int            RelativeY = 0x0100;
    int            ConstantY = 0x0000;
    int            OffsetX = 0x0000;
    int            OffsetY = 0x0000;
    bool           UseDeltaCameraX = false;
    bool           UseDeltaCameraY = false;
    Uint32*        Tiles = NULL;
    Uint32*        TilesBackup = NULL;
    Uint16*        TileOffsetY = NULL;
    int            DeformOffsetA = 0;
    int            DeformOffsetB = 0;
    int            DeformSetA[MAX_DEFORM_LINES];
    int            DeformSetB[MAX_DEFORM_LINES];
    int            DeformSplitLine = 0;
    int            Flags = 0x0000;
    int            DrawGroup = 0;
    Uint8          DrawBehavior = 0;
    bool           Blending = false;
    Uint8          BlendMode = 0; // BlendMode_NORMAL
    float          Opacity = 1.0f;
    bool           UsingCustomScanlineFunction = false;
    ObjFunction    CustomScanlineFunction;
    int            ScrollInfoCount = 0;
    ScrollingInfo* ScrollInfos = NULL;
    int            ScrollInfosSplitIndexesCount = 0;
    Uint16*        ScrollInfosSplitIndexes = NULL;
    Uint8*         ScrollIndexes = NULL;
    Uint32         BufferID = 0;
    int            VertexCount = 0;
    void*          TileBatches = NULL;
    enum {
    FLAGS_COLLIDEABLE = 1,
    FLAGS_NO_REPEAT_X = 2,
    FLAGS_NO_REPEAT_Y = 4,
    }; 

            SceneLayer();
            SceneLayer(int w, int h);
    void    Dispose();
};

#endif /* ENGINE_SCENE_SCENELAYER_H */
