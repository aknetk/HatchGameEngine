#ifndef HTTP_H
#define HTTP_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Includes/Standard.h>
#include <Engine/Includes/StandardSDL2.h>
#include <Engine/Bytecode/Types.h>

class HTTP {
public:
    static bool GET(const char* url, Uint8** outBuf, size_t* outLen, ObjBoundMethod* callback);
};

#endif /* HTTP_H */
