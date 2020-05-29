#ifndef ENGINE_HASHING_CRC32_H
#define ENGINE_HASHING_CRC32_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED


#include <Engine/Includes/Standard.h>

class CRC32 {
public:
    static Uint32 EncryptString(char* data);
    static Uint32 EncryptString(const char* message);
    static Uint32 EncryptData(void* data, Uint32 size);
    static Uint32 EncryptData(void* data, Uint32 size, Uint32 crc);
};

#endif /* ENGINE_HASHING_CRC32_H */
