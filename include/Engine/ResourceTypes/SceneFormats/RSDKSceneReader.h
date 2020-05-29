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
    static bool             Initialized;

    static bool Read(const char* filename, const char* parentFolder);
};

#endif /* ENGINE_RESOURCETYPES_SCENEFORMATS_RSDKSCENEREADER_H */
