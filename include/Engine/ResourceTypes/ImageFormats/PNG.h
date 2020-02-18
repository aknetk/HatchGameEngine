#ifndef PNG_H
#define PNG_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Includes/Standard.h>
#include <Engine/ResourceTypes/ImageFormats/ImageFormat.h>
#include <Engine/IO/Stream.h>

class PNG : public ImageFormat {
public:
    static  PNG*   Load(const char* filename);
    static  bool   Save(PNG* png, const char* filename);
           bool    Save(const char* filename);
                   ~PNG();
};

#endif /* PNG_H */
