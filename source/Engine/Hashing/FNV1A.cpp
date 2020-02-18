#if INTERFACE

#include <Engine/Includes/Standard.h>

class FNV1A {
public:

};
#endif

#include <Engine/Hashing/FNV1A.h>

PUBLIC STATIC Uint32 FNV1A::EncryptString(char* message) {
    Uint32 hash = 0x811C9DC5U;

    while (*message) {
        hash ^= *message;
        hash *= 0x1000193U;
        message++;
    }
    return hash;
}
PUBLIC STATIC Uint32 FNV1A::EncryptString(const char* message) {
    return FNV1A::EncryptString((char*)message);
}

PUBLIC STATIC Uint32 FNV1A::EncryptData(const void* data, Uint32 size) {
    return FNV1A::EncryptData(data, size, 0x811C9DC5U);
}
PUBLIC STATIC Uint32 FNV1A::EncryptData(const void* data, Uint32 size, Uint32 hash) {
    int i = size;
    char* message = (char*)data;
    while (i) {
        hash ^= *message;
        hash *= 0x1000193U;
        message++;
        i--;
    }
    return hash;
}
