#if INTERFACE

#include <Engine/Includes/Standard.h>
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
    int            DrawGroup = 0;

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
};
#endif

#include <Engine/Scene/SceneLayer.h>
#include <Engine/Diagnostics/Memory.h>

PUBLIC         SceneLayer::SceneLayer() {

}
PUBLIC         SceneLayer::SceneLayer(int w, int h) {
    Width = w;
    Height = h;

    w--;
    w |= w >> 1;
    w |= w >> 2;
    w |= w >> 4;
    w |= w >> 8;
    w |= w >> 16;
    w++;
    WidthMask = w - 1;
    WidthInBits = 0;
    for (int f = w >> 1; f > 0; f >>= 1)
        WidthInBits++;

    h--;
    h |= h >> 1;
    h |= h >> 2;
    h |= h >> 4;
    h |= h >> 8;
    h |= h >> 16;
    h++;
    HeightMask = h - 1;
    HeightInBits = 0;
    for (int f = h >> 1; f > 0; f >>= 1)
        HeightInBits++;

    WidthData = w;
    HeightData = h;
    DataSize = w * h * sizeof(Uint32);

    Tiles = (Uint32*)Memory::TrackedCalloc("SceneLayer::Tiles", w * h, sizeof(Uint32));
    TilesBackup = (Uint32*)Memory::TrackedCalloc("SceneLayer::TilesBackup", w * h, sizeof(Uint32));
    // TileOffsetY = (Uint16*)Memory::Calloc(w, sizeof(Uint16));
    Deform = (Sint8*)Memory::Malloc(h * 16 * sizeof(Sint8));
    ScrollIndexes = (Uint8*)Memory::Calloc(h * 16, sizeof(Uint8));
    // memset(ScrollIndexes, 0xFF, h * 16 * sizeof(Uint8));
}
PUBLIC void    SceneLayer::Dispose() {
    if (ScrollInfos)
        Memory::Free(ScrollInfos);
    if (ScrollInfosSplitIndexes)
        Memory::Free(ScrollInfosSplitIndexes);

    // BUG: Somehow Tiles & TilesBackup can be NULL?
    Memory::Free(Tiles);
    Memory::Free(TilesBackup);
    // Memory::Free(TileOffsetY);
    Memory::Free(Deform);
    Memory::Free(ScrollIndexes);
}
