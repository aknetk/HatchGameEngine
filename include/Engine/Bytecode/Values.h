#ifndef ENGINE_BYTECODE_VALUES_H
#define ENGINE_BYTECODE_VALUES_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED


#include <Engine/Includes/Standard.h>
#include <Engine/Bytecode/Types.h>

class Values {
public:
    static bool PrettyPrint;

    static void PrintValue(VMValue value);
    static void PrintValue(PrintBuffer* buffer, VMValue value);
    static void PrintValue(PrintBuffer* buffer, VMValue value, int indent);
    static void PrintObject(PrintBuffer* buffer, VMValue value, int indent);
    static bool GetMethodFromClass(VMValue* value);
};

#endif /* ENGINE_BYTECODE_VALUES_H */
