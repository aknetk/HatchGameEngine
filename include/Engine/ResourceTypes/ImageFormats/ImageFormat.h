#ifndef IMAGEFORMAT_H
#define IMAGEFORMAT_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Includes/Standard.h>

class ImageFormat {
public:
    Uint32* Colors = NULL;
    Uint32* Data = NULL;
    Uint32  Width = 0;
    Uint32  Height = 0;
    Uint32  TransparentColorIndex = 0;

    virtual bool Save(const char* filename);
    virtual      ~ImageFormat();
};

#endif /* IMAGEFORMAT_H */
