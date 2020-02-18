#ifndef IMAGE_H
#define IMAGE_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL

class Texture;

#include <Engine/Includes/Standard.h>
#include <Engine/Rendering/Texture.h>

class Image {
public:
    char              Filename[256];
    Texture*          TexturePtr = NULL;

    Image(const char* filename);
    void Dispose();
    ~Image();
    static Texture* LoadTextureFromResource(const char* filename);
};

#endif /* IMAGE_H */
