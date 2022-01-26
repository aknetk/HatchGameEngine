#ifndef ENGINE_RESOURCETYPES_SCENEFORMATS_RSDKSCENEREADER_H
#define ENGINE_RESOURCETYPES_SCENEFORMATS_RSDKSCENEREADER_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED


#include <Engine/IO/Stream.h>

class RSDKSceneReader {
public:
    static Uint32           Magic;
    static bool             Initialized;

    static void StageConfig_GetColors(const char* filename);
    static void GameConfig_GetColors(const char* filename);
    static bool Read(const char* filename, const char* parentFolder);
    static bool Read(Stream* r, const char* parentFolder);
};

#endif /* ENGINE_RESOURCETYPES_SCENEFORMATS_RSDKSCENEREADER_H */
