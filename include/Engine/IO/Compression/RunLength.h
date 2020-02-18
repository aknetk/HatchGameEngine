#ifndef RUNLENGTH_H
#define RUNLENGTH_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Includes/Standard.h>

class RunLength {
public:
    static bool Decompress(uint8_t* in, size_t in_sz, uint8_t* out, size_t out_sz);
};

#endif /* RUNLENGTH_H */
