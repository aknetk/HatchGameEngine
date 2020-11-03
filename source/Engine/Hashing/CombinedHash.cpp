#if INTERFACE

#include <Engine/Includes/Standard.h>

class CombinedHash {
public:

};
#endif

#include <Engine/Hashing/CombinedHash.h>

#include <Engine/Hashing/MD5.h>
#include <Engine/Hashing/CRC32.h>

PUBLIC STATIC Uint32 CombinedHash::EncryptString(char* data) {
    Uint8 objMD5[16];
    return CRC32::EncryptData(MD5::EncryptString(objMD5, data), 16);
}
PUBLIC STATIC Uint32 CombinedHash::EncryptString(const char* message) {
    return CombinedHash::EncryptString((char*)message);
}

PUBLIC STATIC Uint32 CombinedHash::EncryptData(const void* message, Uint32 len) {
    Uint8 objMD5[16];
    return CRC32::EncryptData(MD5::EncryptData(objMD5, (void*)message, len), 16);
}
