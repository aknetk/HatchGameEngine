#ifndef ENGINE_HASHING_MURMUR_H
#define ENGINE_HASHING_MURMUR_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED


#include <Engine/Includes/Standard.h>

class Murmur {
public:
    static Uint32 EncryptString(char* message);
    static Uint32 EncryptString(const char* message);
    static Uint32 EncryptData(const void* data, Uint32 size);
    static Uint32 EncryptData(const void* key, Uint32 size, Uint32 hash);
};

#endif /* ENGINE_HASHING_MURMUR_H */
