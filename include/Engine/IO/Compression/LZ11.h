#ifndef LZ11_H
#define LZ11_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Includes/Standard.h>

class LZ11 {
public:
    static bool Decompress(uint8_t* in, size_t in_sz, uint8_t* out, size_t out_sz);
};

#endif /* LZ11_H */
