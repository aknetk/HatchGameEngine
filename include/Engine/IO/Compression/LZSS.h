#ifndef LZSS_H
#define LZSS_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Includes/Standard.h>

class LZSS {
public:
    static bool Decompress(uint8_t* in, size_t in_sz, uint8_t* out, size_t out_sz);
};

#endif /* LZSS_H */
