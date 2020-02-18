#ifndef STANDARDLIBRARY_H
#define STANDARDLIBRARY_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Bytecode/Types.h>

class StandardLibrary {
public:
    static int       GetInteger(VMValue* args, int index);
    static float     GetDecimal(VMValue* args, int index);
    static char*     GetString(VMValue* args, int index);
    static ObjArray* GetArray(VMValue* args, int index);
    static void      CheckArgCount(int argCount, int expects);
    static void      CheckAtLeastArgCount(int argCount, int expects);
    static void Link();
    static void Dispose();
};

#endif /* STANDARDLIBRARY_H */
