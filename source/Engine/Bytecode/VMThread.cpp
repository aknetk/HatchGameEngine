#if INTERFACE
#include <Engine/Includes/Standard.h>
#include <Engine/Bytecode/Types.h>

class VMThread {
public:
    struct WithIter {
        void* entity;
        void* entityNext;
        int   index;
        void* list;
    };

    VMValue   Stack[STACK_SIZE_MAX];
    VMValue*  StackTop = Stack;
    VMValue   RegisterValue;

    CallFrame Frames[FRAMES_MAX];
    Uint32    FrameCount;
    Uint32    ReturnFrame;

    VMValue   WithReceiverStack[16];
    VMValue*  WithReceiverStackTop = WithReceiverStack;
    WithIter  WithIteratorStack[16];
    WithIter* WithIteratorStackTop = WithIteratorStack;

    enum ThreadState {
        CREATED = 0,
        RUNNING = 1,
        WAITING = 2,
        CLOSED = 3,
    };

    char      Name[THREAD_NAME_MAX];
    Uint32    ID;
    Uint32    State;
    bool      DebugInfo;

    static    bool InstructionIgnoreMap[0x100];
    static    std::jmp_buf JumpBuffer;
};
#endif

#include <Engine/Bytecode/VMThread.h>
#include <Engine/Bytecode/BytecodeObject.h>
#include <Engine/Bytecode/BytecodeObjectManager.h>
#include <Engine/Bytecode/Compiler.h>
#include <Engine/Bytecode/Values.h>

// Locks are only in 3 places:
// Heap, which contains object memory and globals
// Bytecode area, which contains function bytecode
// Tokens & Strings

#define __Tokens__ BytecodeObjectManager::Tokens

bool         VMThread::InstructionIgnoreMap[0x100];
std::jmp_buf VMThread::JumpBuffer;

// #region Error Handling & Debug Info
char GetTokenBuffer[16];
PUBLIC char*   VMThread::GetToken(Uint32 hash) {
    if (__Tokens__ && __Tokens__->Exists(hash))
        return __Tokens__->Get(hash);

    snprintf(GetTokenBuffer, 15, "%X", hash);
    return &GetTokenBuffer[0];
}
PUBLIC int     VMThread::ThrowRuntimeError(bool fatal, const char* errorMessage, ...) {
    if (VMThread::InstructionIgnoreMap[000000000])
        return ERROR_RES_CONTINUE;

    CallFrame* frame = &Frames[FrameCount - 1];

    va_list args;
    char errorString[512];
    va_start(args, errorMessage);
    vsnprintf(errorString, 512, errorMessage, args);
    va_end(args);

    int line;
    char* source;
    ObjFunction* function = frame->Function;

    char* textBuffer = (char*)malloc(512);

    PrintBuffer buffer;
    buffer.Buffer = &textBuffer;
    buffer.WriteIndex = 0;
    buffer.BufferSize = 512;

    if (function) {
        if (function->Chunk.Lines) {
            int bpos = (frame->IPLast - frame->IPStart);
            line = function->Chunk.Lines[bpos] & 0xFFFF;

            buffer_printf(&buffer, "In event %s of object %s, line %d:\n\n    %s\n", GetToken(function->NameHash), function->SourceFilename, line, errorString);
        }
        else {
            buffer_printf(&buffer, "In %d:\n    %s\n", (int)(frame->IP - frame->IPStart), errorString);
        }
    }
    else {
        buffer_printf(&buffer, "In %d:\n    %s\n", (int)(frame->IP - frame->IPStart), errorString);
    }

    buffer_printf(&buffer, "\nCall Backtrace (Thread %d):\n", ID);
    for (int i = FrameCount - 1; i >= 0; i--) {
        CallFrame* fr = &Frames[i];
        function = fr->Function;
        source = function->SourceFilename;
        line = -1;
        if (i > 0) {
            CallFrame* fr2 = &Frames[i - 1];
            line = fr2->Function->Chunk.Lines[fr2->IPLast - fr2->IPStart] & 0xFFFF;
        }
        buffer_printf(&buffer, "    event %s of object %s", GetToken(function->NameHash), source);

        if (line < 0)
            buffer_printf(&buffer, "\n");
        else
            buffer_printf(&buffer, " (line %d), from\n", line);
    }

    Log::Print(Log::LOG_ERROR, textBuffer);

    PrintStack();

    const SDL_MessageBoxButtonData buttonsError[] = {
        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Exit Game" },
        { 0                                      , 2, "Ignore All" },
        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Continue" },
    };
    const SDL_MessageBoxButtonData buttonsFatal[] = {
        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Exit Game" },
    };

    const SDL_MessageBoxData messageBoxData = {
        SDL_MESSAGEBOX_ERROR, NULL,
        "Script Error",
        textBuffer,
        (int)(fatal ? SDL_arraysize(buttonsFatal) : SDL_arraysize(buttonsError)),
        fatal ? buttonsFatal : buttonsError,
        NULL,
    };

    int buttonClicked;
    if (SDL_ShowMessageBox(&messageBoxData, &buttonClicked) < 0) {
        buttonClicked = 2;
    }

    free(textBuffer);

    switch (buttonClicked) {
        // Exit Game
        case 1:
            exit(-1);
            // NOTE: This is for later, this doesn't actually execute.
            return ERROR_RES_EXIT;
        // Ignore All
        case 2:
            VMThread::InstructionIgnoreMap[000000000] = true;
            return ERROR_RES_CONTINUE;
    }

    return ERROR_RES_CONTINUE;
}
PUBLIC void    VMThread::PrintStack() {
    int i = 0;
    printf("Stack:\n");
    for (VMValue* v = StackTop - 1; v >= Stack; v--) {
        printf("%4d '", i);
        Compiler::PrintValue(*v);
        printf("'\n");
        i--;
    }
}
PUBLIC void    VMThread::ReturnFromNative() {
    std::longjmp(VMThread::JumpBuffer, 1);
}
// #endregion

// #region Stack stuff
PUBLIC void    VMThread::Push(VMValue value) {
    if (StackSize() == STACK_SIZE_MAX) {
        if (ThrowRuntimeError(true, "Stack overflow! \nStack Top: %p \nStack: %p\nCount: %d", StackTop, Stack, StackSize()) == ERROR_RES_CONTINUE)
            return;
    }

    // bool debugInstruction = ID == 1;
    // if (debugInstruction) printf("push\n");

    // if (IS_OBJECT(value)) {
    //     if (AS_OBJECT(value) == NULL) {
    //         ThrowRuntimeError(true, "hol up");
    //     }
    // }
    *(StackTop++) = value;
}
PUBLIC VMValue VMThread::Pop() {
    if (StackTop == Stack) {
        if (ThrowRuntimeError(true, "Stack underflow!") == ERROR_RES_CONTINUE)
            return *StackTop;
    }

    // bool debugInstruction = ID == 1;
    // if (debugInstruction) printf("pop\n");

    StackTop--;
    return *StackTop;
}
PUBLIC VMValue VMThread::Peek(int offset) {
    return *(StackTop - offset - 1);
}
PUBLIC Uint32  VMThread::StackSize() {
    return (Uint32)(StackTop - Stack);
}
PUBLIC void    VMThread::ResetStack() {
    // bool debugInstruction = ID == 1;
    // if (debugInstruction) printf("reset stack\n");

    StackTop = Stack;
}
// #endregion

// #region Instruction stuff
enum   ThreadReturnCodes {
    INTERPRET_RUNTIME_ERROR = -100,
    INTERPRET_GLOBAL_DOES_NOT_EXIST,
    INTERPRET_GLOBAL_ALREADY_EXIST,
    INTERPRET_FINISHED = -1,
    INTERPRET_OK = 0,
};

// NOTE: These should be inlined
PUBLIC Uint8   VMThread::ReadByte(CallFrame* frame) {
    frame->IP += sizeof(Uint8);
    return *(Uint8*)(frame->IP - sizeof(Uint8));
}
PUBLIC Uint16  VMThread::ReadUInt16(CallFrame* frame) {
    frame->IP += sizeof(Uint16);
    return *(Uint16*)(frame->IP - sizeof(Uint16));
}
PUBLIC Uint32  VMThread::ReadUInt32(CallFrame* frame) {
    frame->IP += sizeof(Uint32);
    return *(Uint32*)(frame->IP - sizeof(Uint32));
}
PUBLIC Sint16  VMThread::ReadSInt16(CallFrame* frame) {
    return (Sint16)ReadUInt16(frame);
}
PUBLIC Sint32  VMThread::ReadSInt32(CallFrame* frame) {
    return (Sint32)ReadUInt32(frame);
}
PUBLIC VMValue VMThread::ReadConstant(CallFrame* frame) {
    return (*frame->Function->Chunk.Constants)[ReadUInt32(frame)];
}

PUBLIC int     VMThread::RunInstruction() {
    CallFrame* frame;
    Uint8 instruction;

    frame = &Frames[FrameCount - 1];
    frame->IPLast = frame->IP;

    DebugInfo = ID == 1 && false;
    if (DebugInfo) {
        #define PRINT_CASE(n) case n: Log::Print(Log::LOG_VERBOSE, #n); break;

        switch (*frame->IP) {
            PRINT_CASE(OP_ERROR)
            PRINT_CASE(OP_CONSTANT)
            PRINT_CASE(OP_DEFINE_GLOBAL)
            PRINT_CASE(OP_GET_PROPERTY)
            PRINT_CASE(OP_SET_PROPERTY)
            PRINT_CASE(OP_GET_GLOBAL)
            PRINT_CASE(OP_SET_GLOBAL)
            PRINT_CASE(OP_GET_LOCAL)
            PRINT_CASE(OP_SET_LOCAL)
            PRINT_CASE(OP_PRINT_STACK)
            PRINT_CASE(OP_INHERIT)
            PRINT_CASE(OP_RETURN)
            PRINT_CASE(OP_METHOD)
            PRINT_CASE(OP_CLASS)
            PRINT_CASE(OP_CALL)
            PRINT_CASE(OP_SUPER)
            PRINT_CASE(OP_INVOKE)
            PRINT_CASE(OP_JUMP)
            PRINT_CASE(OP_JUMP_IF_FALSE)
            PRINT_CASE(OP_JUMP_BACK)
            PRINT_CASE(OP_POP)
            PRINT_CASE(OP_COPY)
            PRINT_CASE(OP_ADD)
            PRINT_CASE(OP_SUBTRACT)
            PRINT_CASE(OP_MULTIPLY)
            PRINT_CASE(OP_DIVIDE)
            PRINT_CASE(OP_MODULO)
            PRINT_CASE(OP_NEGATE)
            PRINT_CASE(OP_INCREMENT)
            PRINT_CASE(OP_DECREMENT)
            PRINT_CASE(OP_BITSHIFT_LEFT)
            PRINT_CASE(OP_BITSHIFT_RIGHT)
            PRINT_CASE(OP_NULL)
            PRINT_CASE(OP_TRUE)
            PRINT_CASE(OP_FALSE)
            PRINT_CASE(OP_BW_NOT)
            PRINT_CASE(OP_BW_AND)
            PRINT_CASE(OP_BW_OR)
            PRINT_CASE(OP_BW_XOR)
            PRINT_CASE(OP_LG_NOT)
            PRINT_CASE(OP_LG_AND)
            PRINT_CASE(OP_LG_OR)
            PRINT_CASE(OP_EQUAL)
            PRINT_CASE(OP_EQUAL_NOT)
            PRINT_CASE(OP_GREATER)
            PRINT_CASE(OP_GREATER_EQUAL)
            PRINT_CASE(OP_LESS)
            PRINT_CASE(OP_LESS_EQUAL)
            PRINT_CASE(OP_PRINT)
            PRINT_CASE(OP_ENUM)
            PRINT_CASE(OP_SAVE_VALUE)
            PRINT_CASE(OP_LOAD_VALUE)
            PRINT_CASE(OP_WITH)
            PRINT_CASE(OP_GET_ELEMENT)
            PRINT_CASE(OP_SET_ELEMENT)
            PRINT_CASE(OP_NEW_ARRAY)
            PRINT_CASE(OP_NEW_MAP)
            PRINT_CASE(OP_SWITCH_TABLE)

            default:
                Log::Print(Log::LOG_ERROR, "Unknown opcode %d\n", frame->IP); break;
        }

        #undef  PRINT_CASE
    }

    switch (instruction = ReadByte(frame)) {
        // Globals (heap)
        case OP_GET_GLOBAL: {
            Uint32 hash = ReadUInt32(frame);
            if (BytecodeObjectManager::Lock()) {
                if (!BytecodeObjectManager::Globals->Exists(hash)) {
                    if (__Tokens__ && __Tokens__->Exists(hash)) {
                        if (ThrowRuntimeError(false, "Variable \"%s\" does not exist.", __Tokens__->Get(hash)) == ERROR_RES_CONTINUE)
                            goto FAIL_OP_GET_GLOBAL;
                    }
                    else {
                        if (ThrowRuntimeError(false, "Variable $[%08X] does not exist.", hash) == ERROR_RES_CONTINUE)
                            goto FAIL_OP_GET_GLOBAL;
                    }
                    Push(NULL_VAL);
                    return INTERPRET_GLOBAL_DOES_NOT_EXIST;
                }

                Push(BytecodeObjectManager::DelinkValue(BytecodeObjectManager::Globals->Get(hash)));
                BytecodeObjectManager::Unlock();
            }
            break;

            FAIL_OP_GET_GLOBAL:
            BytecodeObjectManager::Unlock();
            Push(NULL_VAL);
            break;
        }
        case OP_SET_GLOBAL: {
            Uint32 hash = ReadUInt32(frame);
            if (BytecodeObjectManager::Lock()) {
                if (!BytecodeObjectManager::Globals->Exists(hash)) {
                    if (__Tokens__ && __Tokens__->Exists(hash)) {
                        if (ThrowRuntimeError(false, "Global variable \"%s\" does not exist.", __Tokens__->Get(hash)) == ERROR_RES_CONTINUE)
                            goto FAIL_OP_SET_GLOBAL;
                    }
                    else {
                        if (ThrowRuntimeError(false, "Global variable $[%08X] does not exist.", hash) == ERROR_RES_CONTINUE)
                            goto FAIL_OP_SET_GLOBAL;
                    }
                    return INTERPRET_GLOBAL_DOES_NOT_EXIST;
                }

                VMValue LHS = BytecodeObjectManager::Globals->Get(hash);
                VMValue value = Peek(0);
                switch (LHS.Type) {
                    case VAL_LINKED_INTEGER:
                        AS_LINKED_INTEGER(LHS) = AS_INTEGER(value);
                        break;
                    case VAL_LINKED_DECIMAL:
                        AS_LINKED_DECIMAL(LHS) = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(value));
                        break;
                    default:
                        BytecodeObjectManager::Globals->Put(hash, value);
                }
                BytecodeObjectManager::Unlock();
            }
            break;

            FAIL_OP_SET_GLOBAL:
            BytecodeObjectManager::Unlock();
            break;
        }
        case OP_DEFINE_GLOBAL: {
            Uint32 hash = ReadUInt32(frame);
            if (BytecodeObjectManager::Lock()) {
                VMValue value = Peek(0);
                // If it already exists,
                if (BytecodeObjectManager::Globals->Exists(hash)) {
                    VMValue originalValue = BytecodeObjectManager::Globals->Get(hash);

                    // If the value is a class and original is a class,
                    if (IS_CLASS(value) && IS_CLASS(originalValue)) {
                        ObjClass* src = AS_CLASS(value);
                        ObjClass* dst = AS_CLASS(originalValue);

                        src->Methods->WithAll([dst](Uint32 hash, VMValue value) -> void {
                            dst->Methods->Put(hash, value);
                        });
                        src->Methods->Clear();
                    }
                    // Otherwise,
                    else {
                        BytecodeObjectManager::Globals->Put(hash, value);
                    }

                    //     VMValue originalValue = BytecodeObjectManager::Globals->Get(hash);
                    //     // If the value is a class and original is a class,
                    //     if (IS_CLASS(value) && IS_CLASS(originalValue)) {
                    //         ObjClass* src = AS_CLASS(value);
                    //         ObjClass* dst = AS_CLASS(originalValue);
                    //         src->Methods->WithAll([dst](Uint32 hash, VMValue value) -> void {
                    //             dst->Methods->Put(hash, value);
                    //         });
                    //         // TODO: free source class here
                    //         printf("combining classes\n");
                    //     }
                    //     // Otherwise,
                    //     else {
                    //         BytecodeObjectManager::Globals->Put(hash, value);
                    //     }
                    // }
                }
                // Otherwise,
                else {
                    BytecodeObjectManager::Globals->Put(hash, value);
                }
                Pop();
                BytecodeObjectManager::Unlock();
            }
            break;

            // FAIL_OP_DEFINE_GLOBAL:
            // Pop();
            // BytecodeObjectManager::Unlock();
            // break;
        }

        // Object Properties (heap)
        case OP_GET_PROPERTY: {
            Uint32 hash = ReadUInt32(frame);

            VMValue object;
            ObjInstance* instance;

            object = Peek(0);
            if (!IS_INSTANCE(object)) {
                if (ThrowRuntimeError(false, "Only instances have properties.") == ERROR_RES_CONTINUE)
                    goto FAIL_OP_GET_PROPERTY;
            }
            instance = AS_INSTANCE(object);

            if (BytecodeObjectManager::Lock()) {
                if (instance->Fields->Exists(hash)) {
                    Pop();
                    Push(BytecodeObjectManager::DelinkValue(instance->Fields->Get(hash)));
                    BytecodeObjectManager::Unlock();
                    break;
                }

                if (BindMethod(instance->Class, hash)) {
                    BytecodeObjectManager::Unlock();
                    break;
                }

                if (__Tokens__ && __Tokens__->Exists(hash)) {
                    if (ThrowRuntimeError(false, "Could not find %s in instance!", __Tokens__->Get(hash)) == ERROR_RES_CONTINUE)
                        goto FAIL_OP_GET_PROPERTY;
                }
                else {
                    if (ThrowRuntimeError(false, "Could not find %X in instance!", hash) == ERROR_RES_CONTINUE)
                        goto FAIL_OP_GET_PROPERTY;
                }
                goto FAIL_OP_GET_PROPERTY;
            }
            break;

            FAIL_OP_GET_PROPERTY:
            Pop();
            Push(NULL_VAL);
            BytecodeObjectManager::Unlock();
            break;
        }
        case OP_SET_PROPERTY: {
            Uint32 hash;
            VMValue field;
            VMValue value;
            VMValue object;
            ObjInstance* instance;

            object = Peek(1);

            if (!IS_INSTANCE(object)) {
                if (ThrowRuntimeError(false, "Only instances have properties.") == ERROR_RES_CONTINUE)
                    goto FAIL_OP_SET_PROPERTY;
            }
            instance = AS_INSTANCE(object);

            if (BytecodeObjectManager::Lock()) {
                hash = ReadUInt32(frame);

                value = Pop();
                if (instance->Fields->Exists(hash)) {
                    field = instance->Fields->Get(hash);
                    switch (field.Type) {
                        case VAL_LINKED_INTEGER:
                            AS_LINKED_INTEGER(field) = AS_INTEGER(BytecodeObjectManager::CastValueAsInteger(value));
                            break;
                        case VAL_LINKED_DECIMAL:
                            AS_LINKED_DECIMAL(field) = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(value));
                            break;
                        default:
                            instance->Fields->Put(hash, value);
                    }
                }
                else {
                    instance->Fields->Put(hash, value);
                }

                Pop(); // Instance
                Push(value);
                BytecodeObjectManager::Unlock();
            }
            break;

            FAIL_OP_SET_PROPERTY:
            Pop();
            Pop(); // Instance
            Push(NULL_VAL);
            BytecodeObjectManager::Unlock();
            break;
        }
        case OP_GET_ELEMENT: {
            VMValue at = Pop();
            VMValue obj = Pop();
            if (!IS_OBJECT(obj)) {
                if (ThrowRuntimeError(false, "Cannot get value from non-Array or non-Map.") == ERROR_RES_CONTINUE)
                    goto FAIL_OP_GET_ELEMENT;
            }
            if (IS_ARRAY(obj)) {
                if (!IS_INTEGER(at)) {
                    if (ThrowRuntimeError(false, "Cannot get value from array using non-Integer value as an index.") == ERROR_RES_CONTINUE)
                        goto FAIL_OP_GET_ELEMENT;
                }

                if (BytecodeObjectManager::Lock()) {
                    ObjArray* array = AS_ARRAY(obj);
                    int index = AS_INTEGER(at);
                    if (index < 0 || (Uint32)index >= array->Values->size()) {
                        if (ThrowRuntimeError(false, "Index %d is out of bounds of array of size %d.", index, (int)array->Values->size()) == ERROR_RES_CONTINUE)
                            goto FAIL_OP_GET_ELEMENT;
                    }
                    Push((*array->Values)[index]);
                    BytecodeObjectManager::Unlock();
                }
            }
            else if (IS_MAP(obj)) {
                if (!IS_STRING(at)) {
                    if (ThrowRuntimeError(false, "Cannot get value from map using non-String value as an index.") == ERROR_RES_CONTINUE)
                        goto FAIL_OP_GET_ELEMENT;
                }

                if (BytecodeObjectManager::Lock()) {
                    ObjMap* map = AS_MAP(obj);
                    char* index = AS_CSTRING(at);
                    if (!*index) {
                        if (ThrowRuntimeError(false, "Cannot find value at empty key.") == ERROR_RES_CONTINUE)
                            goto FAIL_OP_GET_ELEMENT;
                    }
                    if (!map->Values->Exists(index)) {
                        goto FAIL_OP_GET_ELEMENT;
                    }

                    Push(map->Values->Get(index));
                    BytecodeObjectManager::Unlock();
                }
            }
            else {
                if (ThrowRuntimeError(false, "Cannot get value from object that's non-Array or non-Map.") == ERROR_RES_CONTINUE)
                    goto FAIL_OP_GET_ELEMENT;
            }
            break;

            FAIL_OP_GET_ELEMENT:
            Push(NULL_VAL);
            BytecodeObjectManager::Unlock();
            break;
        }
        case OP_SET_ELEMENT: {
            VMValue value = Peek(0);
            VMValue at = Peek(1);
            VMValue obj = Peek(2);
            if (!IS_OBJECT(obj)) {
                if (ThrowRuntimeError(false, "Cannot set value in non-Array or non-Map.") == ERROR_RES_CONTINUE)
                    goto FAIL_OP_SET_ELEMENT;
            }

            if (IS_ARRAY(obj)) {
                if (!IS_INTEGER(at)) {
                    if (ThrowRuntimeError(false, "Cannot get value from array using non-Integer value as an index.") == ERROR_RES_CONTINUE)
                        goto FAIL_OP_SET_ELEMENT;
                }

                if (BytecodeObjectManager::Lock()) {
                    ObjArray* array = AS_ARRAY(obj);
                    int index = AS_INTEGER(at);
                    if (index < 0 || (Uint32)index >= array->Values->size()) {
                        if (ThrowRuntimeError(false, "Index %d is out of bounds of array of size %d.", index, (int)array->Values->size()) == ERROR_RES_CONTINUE)
                            goto FAIL_OP_SET_ELEMENT;
                    }
                    (*array->Values)[index] = value;
                    BytecodeObjectManager::Unlock();
                }
            }
            else if (IS_MAP(obj)) {
                if (!IS_STRING(at)) {
                    if (ThrowRuntimeError(false, "Cannot get value from map using non-String value as an index.") == ERROR_RES_CONTINUE)
                        goto FAIL_OP_SET_ELEMENT;
                }

                if (BytecodeObjectManager::Lock()) {
                    ObjMap* map = AS_MAP(obj);
                    char* index = AS_CSTRING(at);
                    if (!*index) {
                        if (ThrowRuntimeError(false, "Cannot find value at empty key.") == ERROR_RES_CONTINUE)
                            goto FAIL_OP_SET_ELEMENT;
                    }

                    map->Values->Put(index, value);
                    map->Keys->Put(index, HeapCopyString(index, strlen(index)));
                    BytecodeObjectManager::Unlock();
                }
            }
            else {
                if (ThrowRuntimeError(false, "Cannot set value in object that's non-Array or non-Map.") == ERROR_RES_CONTINUE)
                    goto FAIL_OP_SET_ELEMENT;
            }

            Pop(); // value
            Pop(); // at
            Pop(); // Array
            Push(value);
            break;

            FAIL_OP_SET_ELEMENT:
            Pop(); // value
            Pop(); // at
            Pop(); // Array
            Push(NULL_VAL);
            BytecodeObjectManager::Unlock();
            break;
        }

        // Locals
        case OP_GET_LOCAL: {
            Uint8 slot = ReadByte(frame);
            Push(frame->Slots[slot]);
            break;
        }
        case OP_SET_LOCAL: {
            Uint8 slot = ReadByte(frame);
            frame->Slots[slot] = Peek(0);
            break;
        }

        // Object Allocations (heap)
        case OP_NEW_ARRAY: {
            Uint32 count = ReadUInt32(frame);
            if (BytecodeObjectManager::Lock()) {
                ObjArray* array = NewArray();
                for (int i = count - 1; i >= 0; i--)
                    array->Values->push_back(Peek(i));
                for (int i = count - 1; i >= 0; i--)
                    Pop();
                Push(OBJECT_VAL(array));
                BytecodeObjectManager::Unlock();
            }
            break;
        }
        case OP_NEW_MAP: {
            Uint32 count = ReadUInt32(frame);
            if (BytecodeObjectManager::Lock()) {
                ObjMap* map = NewMap();
                for (int i = count - 1; i >= 0; i--) {
                    char* keystr = AS_CSTRING(Peek(i * 2 + 1));
                    map->Values->Put(keystr, Peek(i * 2));
                    map->Keys->Put(keystr, HeapCopyString(keystr, strlen(keystr)));
                }
                for (int i = count - 1; i >= 0; i--) {
                    Pop();
                    Pop();
                }
                Push(OBJECT_VAL(map));
                BytecodeObjectManager::Unlock();
            }
            break;
        }

        // Stack constants
        case OP_NULL:           Push(NULL_VAL); break;
        case OP_TRUE:           Push(INTEGER_VAL(1)); break;
        case OP_FALSE:          Push(INTEGER_VAL(0)); break;
        case OP_CONSTANT:       Push(ReadConstant(frame)); break;

        // Switch statements
        case OP_SWITCH_TABLE: {
            Uint16 count = ReadUInt16(frame);
            VMValue switch_value = Pop();

            Uint8* end = frame->IP + ((1 + 2) * count) + 3;

            int default_offset = -1;
            for (int i = 0; i < count; i++) {
                int constant_index = ReadByte(frame);
                int offset = ReadUInt16(frame);
                if (constant_index == 0xFF) {
                    default_offset = offset;
                }
                else {
                    VMValue constant = (*frame->Function->Chunk.Constants)[constant_index];
                    if (BytecodeObjectManager::ValuesSortaEqual(switch_value, constant)) {
                        frame->IP = end + offset;
                        goto JUMPED;
                    }
                }
            }

            if (default_offset > -1) {
                frame->IP = end + default_offset;
            }

            JUMPED:
            break;
        }

        // Stack Operations
        case OP_POP: {
            Pop();
            break;
        }
        case OP_COPY: {
            Uint8 count = ReadByte(frame);
            for (int i = 0; i < count; i++)
                Push(Peek(count - 1));
            break;
        }
        case OP_SAVE_VALUE:     RegisterValue = Pop(); break;
        case OP_LOAD_VALUE:     Push(RegisterValue); break;
        case OP_PRINT: {
            VMValue v = Peek(0);

            char* textBuffer = (char*)malloc(64);

            PrintBuffer buffer;
            buffer.Buffer = &textBuffer;
            buffer.WriteIndex = 0;
            buffer.BufferSize = 64;
            Values::PrintValue(&buffer, v);

            Log::Print(Log::LOG_INFO, textBuffer);

            free(textBuffer);

            Pop();
            break;
        }
        case OP_PRINT_STACK: {
            PrintStack();
            break;
        }

        // Frame stuffs & Returning
        case OP_RETURN: {
            VMValue result = Pop();

            FrameCount--;
            if (FrameCount == ReturnFrame) {
                return INTERPRET_FINISHED;
            }

            StackTop = frame->Slots;
            Push(result);

            frame = &Frames[FrameCount - 1];
            break;
        }

        // Jumping
        case OP_JUMP: {
            Sint32 offset = ReadSInt16(frame);
            frame->IP += offset;
            break;
        }
        case OP_JUMP_BACK: {
            Sint32 offset = ReadSInt16(frame);
            frame->IP -= offset;
            break;
        }
        case OP_JUMP_IF_FALSE: {
            Sint32 offset = ReadSInt16(frame);
            if (BytecodeObjectManager::ValueFalsey(Peek(0))) {
                frame->IP += offset;
            }
            break;
        }

        // Numeric Operations
        case OP_ADD:            Push(Values_Plus());  break;
        case OP_SUBTRACT:       Push(Values_Minus());  break;
        case OP_MULTIPLY:       Push(Values_Multiply());  break;
        case OP_DIVIDE:         Push(Values_Division());  break;
        case OP_MODULO:         Push(Values_Modulo());  break;
        case OP_NEGATE:         Push(Values_Negate());  break;
        case OP_INCREMENT:      Push(Values_Increment()); break;
        case OP_DECREMENT:      Push(Values_Decrement()); break;
        // Bit Operations
        case OP_BITSHIFT_LEFT:  Push(Values_BitwiseLeft());  break;
        case OP_BITSHIFT_RIGHT: Push(Values_BitwiseRight());  break;
        // Bitwise Operations
        case OP_BW_NOT:         Push(Values_BitwiseNOT());  break;
        case OP_BW_AND:         Push(Values_BitwiseAnd());  break;
        case OP_BW_OR:          Push(Values_BitwiseOr());  break;
        case OP_BW_XOR:         Push(Values_BitwiseXor());  break;
        // Logical Operations
        case OP_LG_NOT:         Push(Values_LogicalNOT());  break;
        case OP_LG_AND:         Push(Values_LogicalAND()); break;
        case OP_LG_OR:          Push(Values_LogicalOR()); break;
        // Equality and Comparison Operators
        case OP_EQUAL:          Push(INTEGER_VAL(BytecodeObjectManager::ValuesSortaEqual(Pop(), Pop()))); break;
        case OP_EQUAL_NOT:      Push(INTEGER_VAL(!BytecodeObjectManager::ValuesSortaEqual(Pop(), Pop()))); break;
        case OP_LESS:           Push(Values_LessThan()); break;
        case OP_GREATER:        Push(Values_GreaterThan()); break;
        case OP_LESS_EQUAL:     Push(Values_LessThanOrEqual()); break;
        case OP_GREATER_EQUAL:  Push(Values_GreaterThanOrEqual()); break;

        // Functions
        case OP_WITH: {
            enum {
                WITH_STATE_INIT,
                WITH_STATE_ITERATE,
                WITH_STATE_FINISH,
            };

            int state = ReadByte(frame);
            int offset = ReadSInt16(frame);

            switch (state) {
                case WITH_STATE_INIT: {
                    VMValue receiver = Peek(0);
                    if (receiver.Type == VAL_NULL) {
                        frame->IP += offset;
                        Pop(); // pop receiver
                        break;
                    }
                    else if (IS_STRING(receiver)) {
                        // iterate through objectlist
                        ObjectList* objectList = Scene::ObjectRegistries->Get(AS_CSTRING(receiver));
                        if (!objectList)
                            objectList = Scene::ObjectLists->Get(AS_CSTRING(receiver));

                        Pop(); // pop receiver

                        if (!objectList) {
                            // ThrowRuntimeError(false, "Cannot find object class of name \"%s\".", AS_CSTRING(receiver));
                            frame->IP += offset;
                            break;
                        }

                        int objectListCount = objectList->Count();
                        if (objectListCount == 0) {
                            frame->IP += offset;
                            break;
                        }

                        BytecodeObject* objectStart = NULL;
                        int objectListStartIndex = 0;

                        // If in list,
                        if (!objectList->Registry) {
                            for (Entity* ent = objectList->EntityFirst; ent; ent = ent->NextEntityInList) {
                                if (ent->Active && ent->Interactable) {
                                    objectStart = (BytecodeObject*)ent;
                                    break;
                                }
                            }
                        }
                        // Otherwise in registry,
                        else {
                            for (int o = 0; o < objectListCount; o++) {
                                Entity* ent = objectList->GetNth(o);
                                if (ent && ent->Active && ent->Interactable) {
                                    objectStart = (BytecodeObject*)ent;
                                    objectListStartIndex = o;
                                    break;
                                }
                            }
                        }

                        if (!objectStart) {
                            frame->IP += offset;
                            break;
                        }

                        // Add iterator
                        if (objectList->Registry)
                            *WithIteratorStackTop = WithIter { NULL, NULL, objectListStartIndex, objectList };
                        else
                            *WithIteratorStackTop = WithIter { objectStart, objectStart->NextEntityInList, objectListStartIndex, objectList };
                        WithIteratorStackTop++;

                        // Backup original receiver
                        BytecodeObjectManager::Globals->Put("other", frame->Slots[0]);
                        *WithReceiverStackTop = frame->Slots[0];
                        WithReceiverStackTop++;
                        // Replace receiver
                        frame->Slots[0] = OBJECT_VAL(objectStart->Instance);
                        break;
                    }
                    else if (IS_INSTANCE(receiver)) {
                        // Backup original receiver
                        BytecodeObjectManager::Globals->Put("other", frame->Slots[0]);
                        *WithReceiverStackTop = frame->Slots[0];
                        WithReceiverStackTop++;
                        // Replace receiver
                        frame->Slots[0] = receiver;

                        Pop(); // pop receiver

                        // Add dummy iterator
                        *WithIteratorStackTop = WithIter { NULL, NULL, 0, NULL };
                        WithIteratorStackTop++;
                        break;
                    }
                    break;
                }
                case WITH_STATE_ITERATE: {
                    VMValue originalReceiver = WithReceiverStackTop[-1];
                    // Restore original receiver
                    frame->Slots[0] = originalReceiver;

                    WithIter it = WithIteratorStackTop[-1];

                    ObjectList* list = (ObjectList*)it.list;
                    if (list) {
                        // If in list,
                        if (!list->Registry) {
                            Entity* objectNext = NULL;
                            for (Entity* ent = (Entity*)it.entityNext; ent; ent = ent->NextEntityInList) {
                                if (ent->Active && ent->Interactable) {
                                    objectNext = (BytecodeObject*)ent;
                                    break;
                                }
                            }

                            if (objectNext) {
                                it.entity = objectNext;
                                it.entityNext = objectNext->NextEntityInList;

                                frame->IP -= offset;

                                // Put iterator back onto stack
                                WithIteratorStackTop[-1] = it;

                                // Backup original receiver
                                WithReceiverStackTop[-1] = originalReceiver;
                                // Replace receiver
                                BytecodeObject* object = (BytecodeObject*)it.entity;
                                frame->Slots[0] = OBJECT_VAL(object->Instance);
                            }
                        }
                        else if (list->Registry && ++it.index < list->Count()) {
                            frame->IP -= offset;

                            // Put iterator back onto stack
                            WithIteratorStackTop[-1] = it;

                            // Backup original receiver
                            WithReceiverStackTop[-1] = originalReceiver;
                            // Replace receiver
                            BytecodeObject* object = (BytecodeObject*)list->GetNth(it.index);
                            frame->Slots[0] = OBJECT_VAL(object->Instance);
                        }
                        else {
                            // If we are done
                        }
                    }
                    else {
                        Log::Print(Log::LOG_ERROR, "hey you might need to handle the stack here (receiverStack: %d, iterator: %d)", WithReceiverStackTop - WithReceiverStack, WithIteratorStackTop - WithIteratorStack);
                        PrintStack();
                        assert(false);
                    }
                    break;
                }
                case WITH_STATE_FINISH: {
                    WithReceiverStackTop--;

                    VMValue originalReceiver = *WithReceiverStackTop;
                    frame->Slots[0] = originalReceiver;

                    WithIteratorStackTop--;
                    break;
                }
            }
            break;
        }
        case OP_CALL: {
            int argCount = ReadByte(frame);
            if (!CallValue(Peek(argCount), argCount)) {
                if (ThrowRuntimeError(false, "Could not call value!") == ERROR_RES_CONTINUE)
                    goto FAIL_OP_CALL;
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &Frames[FrameCount - 1];
            break;

            FAIL_OP_CALL:
            break;
        }
        case OP_INVOKE: {
            Uint32 argCount = ReadByte(frame);
            Uint32 hash = ReadUInt32(frame);

            VMValue receiver = Peek(argCount);
            if (IS_CLASS(receiver)) {
                ObjClass* klass = AS_CLASS(receiver);
                if (klass->Methods->Exists(hash)) {
                    VMValue nat = klass->Methods->Get(hash);
                    if (IS_NATIVE(nat)) {
                        if (!CallValue(nat, argCount)) {
                            if (ThrowRuntimeError(false, "Could not call value!", hash) == ERROR_RES_CONTINUE)
                                goto FAIL_OP_INVOKE;

                            return INTERPRET_RUNTIME_ERROR;
                        }

                        // // Get returned value
                        // VMValue returned = Pop();
                        // // Pop class value off the stack.
                        // Pop();
                        // // Put returned value on top of stack.
                        // Push(returned);
                    }
                    else {
                        // ThrowRuntimeError(true, "Cannot get non-native function value from class.");
                        // return INTERPRET_RUNTIME_ERROR;
                        // StackTop[-argCount - 1] = receiver;
                        if (!CallValue(nat, argCount)) {
                            if (ThrowRuntimeError(false, "Could not call value!", hash) == ERROR_RES_CONTINUE)
                                goto FAIL_OP_INVOKE;

                            return INTERPRET_RUNTIME_ERROR;
                        }
                    }
                }
                else {
                    if (__Tokens__ && __Tokens__->Exists(hash))
                        if (ThrowRuntimeError(false, "Event \"%s\" does not exist in class %s.", __Tokens__->Get(hash), __Tokens__->Get(klass->Hash)) == ERROR_RES_CONTINUE)
                            goto FAIL_OP_INVOKE;

                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

            if (!Invoke(hash, argCount)) {
                if (__Tokens__ && __Tokens__->Exists(hash))
                    if (ThrowRuntimeError(false, "Could not invoke %s value!", __Tokens__->Get(hash)) == ERROR_RES_CONTINUE)
                        goto FAIL_OP_INVOKE;

                return INTERPRET_RUNTIME_ERROR;
            }

            frame = &Frames[FrameCount - 1];
            break;

            FAIL_OP_INVOKE:
            break;
        }
        case OP_CLASS: {
            Uint32 hash = ReadUInt32(frame);
            ObjClass* klass = NewClass(hash);
            klass->Extended = ReadByte(frame);

            // if (!__Tokens__ || !__Tokens__->Exists(hash)) {
                char name[256];
                sprintf(name, "%8X", hash);
                klass->Name = CopyString(name, strlen(name));
            // }
            // else {
            //     char* t = __Tokens__->Get(hash);
            //     klass->Name = CopyString(t, strlen(t));
            // }

            Push(OBJECT_VAL(klass));
            break;
        }
        case OP_METHOD: {
            int index = ReadByte(frame);
            Uint32 hash = ReadUInt32(frame);
            BytecodeObjectManager::DefineMethod(index, hash);
            break;
        }

        case OP_FAILSAFE: {
            int offset = ReadUInt16(frame);
            frame->Function->Chunk.Failsafe = frame->IPStart + offset;
            // frame->IP = frame->IPStart + offset;
            break;
        }
    }

    if (DebugInfo) {
        Log::Print(Log::LOG_WARN, "START");
        PrintStack();
    }

    return INTERPRET_OK;
}
PUBLIC void    VMThread::RunInstructionSet() {
    while (true) {
        // if (!BytecodeObjectManager::Lock()) break;

        int ret;
        if ((ret = RunInstruction()) < INTERPRET_OK) {
            if (ret < INTERPRET_FINISHED)
                Log::Print(Log::LOG_ERROR, "ret: %d!!!!!!!!!!!!\n", ret);
            // BytecodeObjectManager::Unlock();
            break;
        }
        // BytecodeObjectManager::Unlock();
    }
}
// #endregion

PUBLIC void    VMThread::RunValue(VMValue value, int argCount) {
    int lastReturnFrame = ReturnFrame;

    ReturnFrame = FrameCount;
    CallValue(value, argCount);
    RunInstructionSet();
    ReturnFrame = lastReturnFrame;
}
PUBLIC void    VMThread::RunFunction(ObjFunction* func, int argCount) {
    int lastReturnFrame = ReturnFrame;

    ReturnFrame = FrameCount;
    Call(func, argCount);
    RunInstructionSet();
    ReturnFrame = lastReturnFrame;
}
PUBLIC void    VMThread::RunInvoke(Uint32 hash, int argCount) {
    VMValue* lastStackTop = StackTop;
    int      lastReturnFrame = ReturnFrame;

    ReturnFrame = FrameCount;

    Invoke(hash, argCount);
    RunInstructionSet();

    ReturnFrame = lastReturnFrame;
    StackTop = lastStackTop;
}

PUBLIC bool    VMThread::BindMethod(ObjClass* klass, Uint32 hash) {
    if (BytecodeObjectManager::Lock()) {
        VMValue method;
        if (!klass->Methods->Exists(hash)) {
            if (!__Tokens__ || !__Tokens__->Exists(hash))
                ThrowRuntimeError(false, "Undefined property $[%08X].", hash);
            else
                ThrowRuntimeError(false, "Undefined property \"%s\".", __Tokens__->Get(hash));
            Pop(); // Instance.
            Push(NULL_VAL);
            return false;
        }

        method = klass->Methods->Get(hash);

        // ObjBoundMethod* bound = NewBoundMethod(Peek(0), AS_FUNCTION(method));
        Pop(); // Instance.
        // Push(OBJECT_VAL(bound));
        Push(method);
        BytecodeObjectManager::Unlock();
        return true;
    }
    return false;
}
PUBLIC bool    VMThread::CallValue(VMValue callee, int argCount) {
    if (BytecodeObjectManager::Lock()) {
        bool result;
        if (IS_OBJECT(callee)) {
            switch (OBJECT_TYPE(callee)) {
                case OBJ_BOUND_METHOD: {
                    ObjBoundMethod* bound = AS_BOUND_METHOD(callee);

                    // if (__Tokens__ && __Tokens__->Exists(bound->Method->NameHash))
                    //     printf("Calling Bound Method: %s, argCount: %d\n", __Tokens__->Get(bound->Method->NameHash), argCount);

                    // Replace the bound method with the receiver so it's in the
                    // right slot when the method is called.
                    StackTop[-argCount - 1] = bound->Receiver;
                    result = Call(bound->Method, argCount);
                    BytecodeObjectManager::Unlock();
                    return result;
                }
                /*
                case OBJ_CLASS: {
                    ObjClass* klass = AS_CLASS(callee);

                    // Create the instance.
                    StackTop[-argCount - 1] = OBJECT_VAL(NewInstance(klass));

                    // Call the initializer, if there is one.
                    VMValue initializer;
                    if (klass->Methods->Exists(klass->Hash)) {
                        initializer = klass->Methods->Get(klass->Hash);
                        return Call(AS_FUNCTION(initializer), argCount);
                    }
                    else if (argCount != 0) {
                        ThrowRuntimeError(true, "Expected no arguments to initializer, got %d.", argCount);
                        return false;
                    }
                    return true;
                }
                //*/
                case OBJ_FUNCTION:
                    result = Call(AS_FUNCTION(callee), argCount);
                    BytecodeObjectManager::Unlock();
                    return result;
                case OBJ_NATIVE: {
                    NativeFn native = AS_NATIVE(callee);

                    VMValue result = NULL_VAL;
                    if (setjmp(VMThread::JumpBuffer) == 0) {
                        result = native(argCount, StackTop - argCount, ID);
                    }
                    // Pop arguments
                    StackTop -= argCount;
                    // Pop receiver / class
                    StackTop -= 1;
                    // Push result
                    Push(result);
                    BytecodeObjectManager::Unlock();
                    return true;
                }
                default:
                    // Do nothing.
                    break;
            }
        }
        BytecodeObjectManager::Unlock();
    }

    // runtimeError("Can only call functions and classes.");
    return false;
}
PUBLIC bool    VMThread::Call(ObjFunction* function, int argCount) {
    if (argCount != function->Arity) {
        if (ThrowRuntimeError(false, "Expected %d arguments to function call, got %d.", function->Arity, argCount) == ERROR_RES_CONTINUE)
            return false;
    }

    // if (__Tokens__ && __Tokens__->Exists(function->NameHash))
    //     printf("new frame '%s' at %d#%d\n", __Tokens__->Get(function->NameHash), ID, FrameCount);

    if (FrameCount == FRAMES_MAX) {
        ThrowRuntimeError(true, "Frame overflow. (Count %d / %d)", FrameCount, FRAMES_MAX);
        return false;
    }

    CallFrame* frame = &Frames[FrameCount++];
    frame->IP = function->Chunk.Code;
    frame->IPStart = frame->IP;
    frame->Function = function;
    frame->Slots = StackTop - (function->Arity + 1);

    return true;
}
PUBLIC bool    VMThread::InvokeFromClass(ObjClass* klass, Uint32 hash, int argCount) {
    if (BytecodeObjectManager::Lock()) {
        VMValue method;
        if (!klass->Methods->Exists(hash)) {
            // if (!__Tokens__ || !__Tokens__->Exists(hash))
            //     ThrowRuntimeError(true, "Undefined property $[%08X].", hash);
            // else
            //     ThrowRuntimeError(true, "Undefined property \"%s\".", __Tokens__->Get(hash));
            BytecodeObjectManager::Unlock();
            return false;
        }
        method = klass->Methods->Get(hash);

        if (IS_NATIVE(method)) {
            NativeFn native = AS_NATIVE(method);
            VMValue result = native(argCount + 1, StackTop - argCount - 1, ID);
            StackTop -= argCount + 1;
            Push(result);
            BytecodeObjectManager::Unlock();
            return true;
        }

        BytecodeObjectManager::Unlock();
        return Call(AS_FUNCTION(method), argCount);
    }
    return false;
}
PUBLIC bool    VMThread::Invoke(Uint32 hash, int argCount) {
    VMValue receiver = Peek(argCount);

    if (!IS_INSTANCE(receiver)) {
        ThrowRuntimeError(false, "Only instances have methods.");
        return false;
    }

    ObjInstance* instance = AS_INSTANCE(receiver);

    // First look for a field which may shadow a method.
    VMValue value;
    bool exists = false;
    if (BytecodeObjectManager::Lock()) {
        if ((exists = instance->Fields->Exists(hash)))
            value = instance->Fields->Get(hash);

        BytecodeObjectManager::Unlock();

        // if (IS_OBJECT(value)) {
        //     printf("instance.%s: ", __Tokens__->Get(hash));
        //     Compiler::PrintValue(value);
        //     printf(" exists: %d argCount: %d\n", exists, argCount);
        //
        //     PrintStack();
        // }
    }
    if (exists) {
        // StackTop[-argCount] = value;
        return CallValue(value, argCount);
    }

    return InvokeFromClass(instance->Class, hash, argCount);
}

// #region Value Operations
// TODO: Move these definitions to Types.h
const char* BasicTypesNames[] = {
    "Null",
    "Integer",
    "Decimal",
    "Object",
    "LinkedInteger",
    "LinkedDecimal",
};
const char* ObjectTypesNames[] = {
    "Bound Method",
    "Class",
    "Closure",
    "Function",
    "Instance",
    "Native",
    "String",
    "Upvalue",
    "Array",
    "Map",
};

#define IS_NOT_NUMBER(a) (a.Type != VAL_DECIMAL && a.Type != VAL_INTEGER && a.Type != VAL_LINKED_DECIMAL && a.Type != VAL_LINKED_INTEGER)
#define CHECK_IS_NUM(a, b) if (IS_NOT_NUMBER(a)) ThrowRuntimeError(false, "Cannot perform %s operation on non-number value of type %s.", #b, a.Type == VAL_OBJECT ? ObjectTypesNames[OBJECT_TYPE(a)] : BasicTypesNames[a.Type]);

enum {
    ASSIGNMENT_MULTIPLY,
    ASSIGNMENT_DIVISION,
    ASSIGNMENT_MODULO,
    ASSIGNMENT_PLUS,
    ASSIGNMENT_MINUS,
    ASSIGNMENT_BITWISE_LEFT,
    ASSIGNMENT_BITWISE_RIGHT,
    ASSIGNMENT_BITWISE_AND,
    ASSIGNMENT_BITWISE_XOR,
    ASSIGNMENT_BITWISE_OR,
};
// If one of the operators is a decimal, they both become one

PUBLIC VMValue VMThread::Values_Multiply() {
    VMValue b = Peek(0);
    VMValue a = Peek(1);

    CHECK_IS_NUM(a, "multiply");
    CHECK_IS_NUM(b, "multiply");

    Pop();
    Pop();

    if (a.Type == VAL_DECIMAL || b.Type == VAL_DECIMAL) {
        float a_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(a));
        float b_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(b));
        return DECIMAL_VAL(a_d * b_d);
    }
    int a_d = AS_INTEGER(a);
    int b_d = AS_INTEGER(b);
    return INTEGER_VAL(a_d * b_d);
}
PUBLIC VMValue VMThread::Values_Division() {
    VMValue b = Pop();
    VMValue a = Pop();

    CHECK_IS_NUM(a, "division");
    CHECK_IS_NUM(b, "division");

    if (a.Type == VAL_DECIMAL || b.Type == VAL_DECIMAL) {
        float a_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(a));
        float b_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(b));
        if (b_d == 0.0) {
            if (ThrowRuntimeError(false, "Cannot divide decimal by zero.") == ERROR_RES_CONTINUE)
                return DECIMAL_VAL(0.f);
        }
        return DECIMAL_VAL(a_d / b_d);
    }
    int a_d = AS_INTEGER(a);
    int b_d = AS_INTEGER(b);
    if (b_d == 0) {
        if (ThrowRuntimeError(false, "Cannot divide integer by zero.") == ERROR_RES_CONTINUE)
            return INTEGER_VAL(0);
    }
    return INTEGER_VAL(a_d / b_d);
}
PUBLIC VMValue VMThread::Values_Modulo() {
    VMValue b = Pop();
    VMValue a = Pop();

    CHECK_IS_NUM(a, "modulo");
    CHECK_IS_NUM(b, "modulo");

    if (a.Type == VAL_DECIMAL || b.Type == VAL_DECIMAL) {
        float a_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(a));
        float b_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(b));
        return DECIMAL_VAL(fmod(a_d, b_d));
    }
    int a_d = AS_INTEGER(a);
    int b_d = AS_INTEGER(b);
    return INTEGER_VAL(a_d % b_d);
}
PUBLIC VMValue VMThread::Values_Plus() {
    VMValue b = Peek(0);
    VMValue a = Peek(1);
    if (IS_STRING(a) || IS_STRING(b)) {
        if (BytecodeObjectManager::Lock()) {
            VMValue str_b = BytecodeObjectManager::CastValueAsString(b);
            VMValue str_a = BytecodeObjectManager::CastValueAsString(a);
            VMValue out = BytecodeObjectManager::Concatenate(str_a, str_b);
            Pop();
            Pop();
            BytecodeObjectManager::Unlock();
            return out;
        }
    }

    CHECK_IS_NUM(a, "plus");
    CHECK_IS_NUM(b, "plus");

    if (a.Type == VAL_DECIMAL || b.Type == VAL_DECIMAL) {
        float a_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(a));
        float b_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(b));
        Pop();
        Pop();
        return DECIMAL_VAL(a_d + b_d);
    }
    int a_d = AS_INTEGER(a);
    int b_d = AS_INTEGER(b);
    Pop();
    Pop();
    return INTEGER_VAL(a_d + b_d);
}
PUBLIC VMValue VMThread::Values_Minus() {
    VMValue b = Peek(0);
    VMValue a = Peek(1);

    CHECK_IS_NUM(a, "minus");
    CHECK_IS_NUM(b, "minus");

    Pop();
    Pop();

    if (a.Type == VAL_DECIMAL || b.Type == VAL_DECIMAL) {
        float a_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(a));
        float b_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(b));
        return DECIMAL_VAL(a_d - b_d);
    }
    int a_d = AS_INTEGER(a);
    int b_d = AS_INTEGER(b);
    return INTEGER_VAL(a_d - b_d);
}
PUBLIC VMValue VMThread::Values_BitwiseLeft() {
    VMValue b = Pop();
    VMValue a = Pop();

    CHECK_IS_NUM(a, "bitwise left");
    CHECK_IS_NUM(b, "bitwise left");

    if (a.Type == VAL_DECIMAL || b.Type == VAL_DECIMAL) {
        float a_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(a));
        float b_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(b));
        return DECIMAL_VAL((float)((int)a_d << (int)b_d));
    }
    int a_d = AS_INTEGER(a);
    int b_d = AS_INTEGER(b);
    return INTEGER_VAL(a_d << b_d);
}
PUBLIC VMValue VMThread::Values_BitwiseRight() {
    VMValue b = Pop();
    VMValue a = Pop();

    CHECK_IS_NUM(a, "bitwise right");
    CHECK_IS_NUM(b, "bitwise right");

    if (a.Type == VAL_DECIMAL || b.Type == VAL_DECIMAL) {
        float a_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(a));
        float b_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(b));
        return DECIMAL_VAL((float)((int)a_d >> (int)b_d));
    }
    int a_d = AS_INTEGER(a);
    int b_d = AS_INTEGER(b);
    return INTEGER_VAL(a_d >> b_d);
}
PUBLIC VMValue VMThread::Values_BitwiseAnd() {
    VMValue b = Pop();
    VMValue a = Pop();

    CHECK_IS_NUM(a, "bitwise and");
    CHECK_IS_NUM(b, "bitwise and");

    if (a.Type == VAL_DECIMAL || b.Type == VAL_DECIMAL) {
        float a_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(a));
        float b_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(b));
        return DECIMAL_VAL((float)((int)a_d & (int)b_d));
    }
    int a_d = AS_INTEGER(a);
    int b_d = AS_INTEGER(b);
    return INTEGER_VAL(a_d & b_d);
}
PUBLIC VMValue VMThread::Values_BitwiseXor() {
    VMValue b = Pop();
    VMValue a = Pop();

    CHECK_IS_NUM(a, "xor");
    CHECK_IS_NUM(b, "xor");

    if (a.Type == VAL_DECIMAL || b.Type == VAL_DECIMAL) {
        float a_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(a));
        float b_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(b));
        return DECIMAL_VAL((float)((int)a_d ^ (int)b_d));
    }
    int a_d = AS_INTEGER(a);
    int b_d = AS_INTEGER(b);
    return INTEGER_VAL(a_d ^ b_d);
}
PUBLIC VMValue VMThread::Values_BitwiseOr() {
    VMValue b = Pop();
    VMValue a = Pop();

    CHECK_IS_NUM(a, "bitwise or");
    CHECK_IS_NUM(b, "bitwise or");

    if (a.Type == VAL_DECIMAL || b.Type == VAL_DECIMAL) {
        float a_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(a));
        float b_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(b));
        return DECIMAL_VAL((float)((int)a_d | (int)b_d));
    }
    int a_d = AS_INTEGER(a);
    int b_d = AS_INTEGER(b);
    return INTEGER_VAL(a_d | b_d);
}
PUBLIC VMValue VMThread::Values_LogicalAND() {
    VMValue b = Pop();
    VMValue a = Pop();

    CHECK_IS_NUM(a, "logical and");
    CHECK_IS_NUM(b, "logical and");

    if (a.Type == VAL_DECIMAL || b.Type == VAL_DECIMAL) {
        // float a_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(a));
        // float b_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(b));
        // return DECIMAL_VAL((float)((int)a_d & (int)b_d));
        return INTEGER_VAL(0);
    }
    int a_d = AS_INTEGER(a);
    int b_d = AS_INTEGER(b);
    return INTEGER_VAL(a_d && b_d);
}
PUBLIC VMValue VMThread::Values_LogicalOR() {
    VMValue b = Pop();
    VMValue a = Pop();

    CHECK_IS_NUM(a, "logical or");
    CHECK_IS_NUM(b, "logical or");

    if (a.Type == VAL_DECIMAL || b.Type == VAL_DECIMAL) {
        // float a_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(a));
        // float b_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(b));
        // return DECIMAL_VAL((float)((int)a_d & (int)b_d));
        return INTEGER_VAL(0);
    }
    int a_d = AS_INTEGER(a);
    int b_d = AS_INTEGER(b);
    return INTEGER_VAL(a_d || b_d);
}
PUBLIC VMValue VMThread::Values_LessThan() {
    VMValue b = Pop();
    VMValue a = Pop();

    CHECK_IS_NUM(a, "less than");
    CHECK_IS_NUM(b, "less than");

    if (a.Type == VAL_DECIMAL || b.Type == VAL_DECIMAL) {
        float a_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(a));
        float b_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(b));
        return INTEGER_VAL(a_d < b_d);
    }
    int a_d = AS_INTEGER(a);
    int b_d = AS_INTEGER(b);
    return INTEGER_VAL(a_d < b_d);
}
PUBLIC VMValue VMThread::Values_GreaterThan() {
    VMValue b = Pop();
    VMValue a = Pop();

    CHECK_IS_NUM(a, "greater than");
    CHECK_IS_NUM(b, "greater than");

    if (a.Type == VAL_DECIMAL || b.Type == VAL_DECIMAL) {
        float a_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(a));
        float b_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(b));
        return INTEGER_VAL(a_d > b_d);
    }
    int a_d = AS_INTEGER(a);
    int b_d = AS_INTEGER(b);
    return INTEGER_VAL(a_d > b_d);
}
PUBLIC VMValue VMThread::Values_LessThanOrEqual() {
    VMValue b = Pop();
    VMValue a = Pop();

    CHECK_IS_NUM(a, "less than or equal");
    CHECK_IS_NUM(b, "less than or equal");

    if (a.Type == VAL_DECIMAL || b.Type == VAL_DECIMAL) {
        float a_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(a));
        float b_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(b));
        return INTEGER_VAL(a_d <= b_d);
    }
    int a_d = AS_INTEGER(a);
    int b_d = AS_INTEGER(b);
    return INTEGER_VAL(a_d <= b_d);
}
PUBLIC VMValue VMThread::Values_GreaterThanOrEqual() {
    VMValue b = Pop();
    VMValue a = Pop();

    CHECK_IS_NUM(a, "greater than or equal");
    CHECK_IS_NUM(b, "greater than or equal");

    if (a.Type == VAL_DECIMAL || b.Type == VAL_DECIMAL) {
        float a_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(a));
        float b_d = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(b));
        return INTEGER_VAL(a_d >= b_d);
    }
    int a_d = AS_INTEGER(a);
    int b_d = AS_INTEGER(b);
    return INTEGER_VAL(a_d >= b_d);
}
PUBLIC VMValue VMThread::Values_Increment() {
    VMValue a = Pop();

    CHECK_IS_NUM(a, "increment");

    if (a.Type == VAL_DECIMAL) {
        float a_d = AS_DECIMAL(a);
        return DECIMAL_VAL(++a_d);
    }
    int a_d = AS_INTEGER(a);
    return INTEGER_VAL(++a_d);
}
PUBLIC VMValue VMThread::Values_Decrement() {
    VMValue a = Pop();

    CHECK_IS_NUM(a, "decrement");

    if (a.Type == VAL_DECIMAL) {
        float a_d = AS_DECIMAL(a);
        return DECIMAL_VAL(--a_d);
    }
    int a_d = AS_INTEGER(a);
    return INTEGER_VAL(--a_d);
}
PUBLIC VMValue VMThread::Values_Negate() {
    VMValue a = Pop();

    CHECK_IS_NUM(a, "negate");

    if (a.Type == VAL_DECIMAL) {
        return DECIMAL_VAL(-AS_DECIMAL(a));
    }
    return INTEGER_VAL(-AS_INTEGER(a));
}
PUBLIC VMValue VMThread::Values_LogicalNOT() {
    VMValue a = Pop();

    // HACK: Yikes.
    switch (a.Type) {
        case VAL_NULL:
            return INTEGER_VAL(true);
        case VAL_OBJECT:
            return INTEGER_VAL(false);
        case VAL_DECIMAL:
        case VAL_LINKED_DECIMAL:
            return DECIMAL_VAL((float)(AS_DECIMAL(a) == 0.0));
        case VAL_INTEGER:
        case VAL_LINKED_INTEGER:
            return INTEGER_VAL(!AS_INTEGER(a));
    }

    return INTEGER_VAL(false);
}
PUBLIC VMValue VMThread::Values_BitwiseNOT() {
    VMValue a = Pop();
    if (a.Type == VAL_DECIMAL) {
        return DECIMAL_VAL((float)(~(int)AS_DECIMAL(a)));
    }
    return INTEGER_VAL(~AS_INTEGER(a));
}
// #endregion
