#ifndef RSDKSCENEREADER_H
#define RSDKSCENEREADER_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/IO/Stream.h>

class RSDKSceneReader {
public:
    static bool             Initialized;

    static bool Read(const char* filename, const char* parentFolder);
};

#endif /* RSDKSCENEREADER_H */
