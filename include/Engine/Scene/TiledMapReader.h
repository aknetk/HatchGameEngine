#ifndef TILEDMAPREADER_H
#define TILEDMAPREADER_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/IO/Stream.h>

class TiledMapReader {
public:
    // static bool             Initialized;

    static void Read(const char* sourceF, const char* parentFolder);
};

#endif /* TILEDMAPREADER_H */
