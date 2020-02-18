#ifndef COMBINEDHASH_H
#define COMBINEDHASH_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Includes/Standard.h>

class CombinedHash {
public:
    static Uint32 EncryptString(char* data);
    static Uint32 EncryptString(const char* message);
    static Uint32 EncryptData(const char* message, size_t len);
};

#endif /* COMBINEDHASH_H */
