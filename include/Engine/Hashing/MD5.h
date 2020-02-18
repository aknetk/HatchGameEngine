#ifndef MD5_H
#define MD5_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Includes/Standard.h>

class MD5 {
private:
    static void* Body(Uint32* pa, Uint32* pb, Uint32* pc, Uint32* pd, void *data, unsigned long size);

public:
    static Uint8* EncryptString(Uint8* dest, char* message);
    static Uint8* EncryptString(Uint8* dest, const char* message);
    static Uint8* EncryptData(Uint8* dest, void* data, Uint32 size);
};

#endif /* MD5_H */
