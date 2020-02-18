#ifndef FNV1A_H
#define FNV1A_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Includes/Standard.h>

class FNV1A {
public:
    static Uint32 EncryptString(char* message);
    static Uint32 EncryptString(const char* message);
    static Uint32 EncryptData(const void* data, Uint32 size);
    static Uint32 EncryptData(const void* data, Uint32 size, Uint32 hash);
};

#endif /* FNV1A_H */
