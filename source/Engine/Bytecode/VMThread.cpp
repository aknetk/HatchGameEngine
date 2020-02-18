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
};
#endif

#include <Engine/Bytecode/VMThread.h>
#include <Engine/Bytecode/BytecodeObject.h>
#include <Engine/Bytecode/BytecodeObjectManager.h>
#include <Engine/Bytecode/Compiler.h>

// Locks are only in 3 places:
// Heap, which contains object memory and globals
// Bytecode area, which contains function bytecode
// Tokens & Strings

#define __Tokens__ BytecodeObjectManager::Tokens

// #region Error Handling & Debug Info
PUBLIC bool    VMThread::ThrowError(bool fatal, const char* errorMessage, ...) {
    CallFrame* frame = &Frames[FrameCount - 1];

    va_list args;
    char errorString[512];
    va_start(args, errorMessage);
    vsnprintf(errorString, 512, errorMessage, args);
    va_end(args);

    int line;
    char* source;
    ObjFunction* function = frame->Function;

    char errorText[4096];
    size_t errorTextLen = 4096 - 1;

    if (function->Chunk.Lines) {
        int bpos = (frame->IPLast - frame->IPStart);
        line = function->Chunk.Lines[bpos] & 0xFFFF;
        // int pos = line >> 16;

        if (__Tokens__ && __Tokens__->Exists(function->NameHash))
            errorTextLen -= snprintf(errorText, errorTextLen, "In event %s of object %s, line %d:\n\n    %s\n", __Tokens__->Get(function->NameHash), function->SourceFilename, line, errorString);
        else
            errorTextLen -= snprintf(errorText, errorTextLen, "In event %X of object %s, line %d:\n\n    %s\n", function->NameHash, function->SourceFilename, line, errorString);
    }
    else {
        errorTextLen -= snprintf(errorText, errorTextLen, "In %d:\n    %s\n", (int)(frame->IP - frame->IPStart), errorString);
    }

    errorTextLen -= snprintf(errorText, errorTextLen, "%s\nCall Backtrace (Thread %d):\n", errorText, ID);
    for (int i = FrameCount - 1; i >= 0; i--) {
        CallFrame* fr = &Frames[i];
        function = fr->Function;
        source = function->SourceFilename;
        line = -1;
        if (i > 0) {
            CallFrame* fr2 = &Frames[i - 1];
            line = fr2->Function->Chunk.Lines[fr2->IPLast - fr2->IPStart] & 0xFFFF;
        }
        if (__Tokens__ && __Tokens__->Exists(function->NameHash))
            errorTextLen -= snprintf(errorText, errorTextLen, "%s    event %s of object %s", errorText, __Tokens__->Get(function->NameHash), source);

        if (line < 0)
            errorTextLen -= snprintf(errorText, errorTextLen, "%s\n", errorText);
        else
            errorTextLen -= snprintf(errorText, errorTextLen, "%s (line %d), from\n", errorText, line);
    }

    Log::Print(Log::LOG_ERROR, errorText);

    PrintStack();

    const SDL_MessageBoxButtonData buttonsError[] = {
        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Exit Game" },
        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Continue" },
    };
    const SDL_MessageBoxButtonData buttonsFatal[] = {
        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Exit Game" },
    };
    const SDL_MessageBoxData messageboxdata = {
        SDL_MESSAGEBOX_ERROR, NULL,
        "Script Error",
        errorText,
        int(fatal ? SDL_arraysize(buttonsFatal) : SDL_arraysize(buttonsError)),
        fatal ? buttonsFatal : buttonsError,
        NULL,
    };
    int buttonid;
    if (SDL_ShowMessageBox(&messageboxdata, &buttonid) < 0) {
        buttonid = 1;
    }

    switch (buttonid) {
        case 1:
            exit(0);
            break;
        default:
            break;
    }

    return false;
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
// #endregion

// #region Stack stuff
PUBLIC void    VMThread::Push(VMValue value) {
    if (StackSize() == STACK_SIZE_MAX) {
        ThrowError(false, "Stack overflow! \nStack Top: %p \nStack: %p\nCount: %d", StackTop, Stack, StackSize());
    }

    // bool debugInstruction = ID == 1;
    // if (debugInstruction) printf("push\n");

    // if (IS_OBJECT(value)) {
    //     if (AS_OBJECT(value) == NULL) {
    //         ThrowError(true, "hol up");
    //     }
    // }
    *(StackTop++) = value;
}
PUBLIC VMValue VMThread::Pop() {
    if (StackTop == Stack) {
        ThrowError(true, "Stack underflow!");
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
    return (*frame->Function->Chunk.Constants)[ReadByte(frame)];
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

            default:
                Log::Print(Log::LOG_ERROR, "Unknown opcode %d\n", frame->IP); break;
        }

        #undef  PRINT_CASE
    }

    switch (instruction = ReadByte(frame)) {
        // Globals (heap)
        case OP_GET_GLOBAL: {
            if (BytecodeObjectManager::Lock()) {
                Uint32 hash = ReadUInt32(frame);
                if (!BytecodeObjectManager::Globals->Exists(hash)) {
                    if (__Tokens__ && __Tokens__->Exists(hash))
                        ThrowError(true, "Variable \"%s\" does not exist.", __Tokens__->Get(hash));
                    else
                        ThrowError(true, "Variable $[%08X] does not exist.", hash);
                    Push(NULL_VAL);
                    return INTERPRET_GLOBAL_DOES_NOT_EXIST;
                }

                Push(BytecodeObjectManager::DelinkValue(BytecodeObjectManager::Globals->Get(hash)));
                BytecodeObjectManager::Unlock();
            }
            break;
        }
        case OP_SET_GLOBAL: {
            if (BytecodeObjectManager::Lock()) {
                Uint32 hash = ReadUInt32(frame);
                if (!BytecodeObjectManager::Globals->Exists(hash)) {
                    if (!__Tokens__ || !__Tokens__->Exists(hash))
                        ThrowError(true, "Global variable $[%08X] does not exist.", hash);
                    else
                        ThrowError(true, "Global variable \"%s\" does not exist.", __Tokens__->Get(hash));
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
        }
        case OP_DEFINE_GLOBAL: {
            if (BytecodeObjectManager::Lock()) {
                Uint32 hash = ReadUInt32(frame);
                BytecodeObjectManager::Globals->Put(hash, Peek(0));
                Pop();
                BytecodeObjectManager::Unlock();
            }
            break;
        }

        // Object Properties (heap)
        case OP_GET_PROPERTY: {
            Uint32 hash;
            VMValue object;
            ObjInstance* instance;

            object = Peek(0);

            if (!IS_INSTANCE(object)) {
                ThrowError(true, "Only instances have properties.");
                Pop();
                Push(NULL_VAL);
                return INTERPRET_RUNTIME_ERROR;
            }
            instance = AS_INSTANCE(object);

            if (BytecodeObjectManager::Lock()) {
                hash = ReadUInt32(frame);
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
                if (__Tokens__ && __Tokens__->Exists(hash))
                    ThrowError(true, "Could not find %s in instance!", __Tokens__->Get(hash));
                else
                    ThrowError(true, "Could not find %X in instance!", hash);
                Pop();
                Push(NULL_VAL);
                BytecodeObjectManager::Unlock();
                return INTERPRET_RUNTIME_ERROR;
            }
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
                ThrowError(true, "Only instances have properties.");
                Push(NULL_VAL);
                return INTERPRET_RUNTIME_ERROR;
            }
            instance = AS_INSTANCE(object);

            if (BytecodeObjectManager::Lock()) {
                hash = ReadUInt32(frame);

				if (__Tokens__ && __Tokens__->Exists(hash)) {
					// printf("OP_SET_PROPERTY: %s\n", __Tokens__->Get(hash));
				}

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
        }
        case OP_GET_ELEMENT: {
            VMValue at = Pop();
            VMValue obj = Pop();
            if (!IS_OBJECT(obj)) {
                ThrowError(true, "Cannot get value from non-Array or non-Map.");
                Push(NULL_VAL);
                break;
            }
            if (IS_ARRAY(obj)) {
                if (!IS_INTEGER(at)) {
                    ThrowError(true, "Cannot get value from array using non-Integer value as an index.");
                    Push(NULL_VAL);
                    break;
                }

                if (BytecodeObjectManager::Lock()) {
                    ObjArray* array = AS_ARRAY(obj);
                    int index = AS_INTEGER(at);
                    if (index < 0 || (Uint32)index >= array->Values->size()) {
                        ThrowError(true, "Index %d is out of bounds of array of size %d.", index, (int)array->Values->size());
                        Push(NULL_VAL);
                        BytecodeObjectManager::Unlock();
                        break;
                    }
                    Push((*array->Values)[index]);
                    BytecodeObjectManager::Unlock();
                }
            }
            else if (IS_MAP(obj)) {
                if (!IS_STRING(at)) {
                    ThrowError(true, "Cannot get value from map using non-String value as an index.");
                    Push(NULL_VAL);
                    break;
                }

                if (BytecodeObjectManager::Lock()) {
                    ObjMap* map = AS_MAP(obj);
                    char* index = AS_CSTRING(at);
                    if (!*index) {
                        ThrowError(true, "Cannot find value at empty key.");
                        Push(NULL_VAL);
                        BytecodeObjectManager::Unlock();
                        break;
                    }
                    if (!map->Values->Exists(index)) {
                        // ThrowError(true, "Cannot find value at key \"%s\".", index);
                        Push(NULL_VAL);
                        BytecodeObjectManager::Unlock();
                        break;
                    }

                    Push(map->Values->Get(index));
                    BytecodeObjectManager::Unlock();
                }
            }
            else {
                ThrowError(true, "Cannot get value from object that's non-Array or non-Map.");
                Push(NULL_VAL);
                break;
            }
            break;
        }
        case OP_SET_ELEMENT: {
            VMValue value = Peek(0);
            VMValue at = Peek(1);
            VMValue obj = Peek(2);
            if (!IS_OBJECT(obj)) {
                ThrowError(true, "Cannot set value in non-Array or non-Map.");
                Pop(); Pop(); Pop();
                Push(NULL_VAL);
                break;
            }

            if (IS_ARRAY(obj)) {
                if (!IS_INTEGER(at)) {
                    ThrowError(true, "Cannot get value from array using non-Integer value as an index.");
                    Pop(); Pop(); Pop();
                    Push(NULL_VAL);
                    break;
                }

                if (BytecodeObjectManager::Lock()) {
                    ObjArray* array = AS_ARRAY(obj);
                    int index = AS_INTEGER(at);
                    if (index < 0 || (Uint32)index >= array->Values->size()) {
                        ThrowError(true, "Index %d is out of bounds of array of size %d.", index, (int)array->Values->size());
                        Pop(); Pop(); Pop();
                        Push(NULL_VAL);
                        break;
                    }
                    (*array->Values)[index] = value;
                    BytecodeObjectManager::Unlock();
                }
            }
            else if (IS_MAP(obj)) {
                if (!IS_STRING(at)) {
                    ThrowError(true, "Cannot get value from map using non-String value as an index.");
                    Pop(); Pop(); Pop();
                    Push(NULL_VAL);
                    break;
                }

                if (BytecodeObjectManager::Lock()) {
                    ObjMap* map = AS_MAP(obj);
                    char* index = AS_CSTRING(at);
                    if (!*index) {
                        ThrowError(true, "Cannot find value at empty key.");
                        Pop(); Pop(); Pop();
                        Push(NULL_VAL);
                        break;
                    }

                    map->Values->Put(index, value);
                    map->Keys->Put(index, HeapCopyString(index, strlen(index)));
                    BytecodeObjectManager::Unlock();
                }
            }
            else {
                ThrowError(true, "Cannot set value in object that's non-Array or non-Map.");
                Pop(); Pop(); Pop();
                Push(NULL_VAL);
                break;
            }

            Pop(); // value
            Pop(); // at
            Pop(); // Array
            Push(value);
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

            char* buffer = (char*)malloc(4);
            int   buffer_info[2] = { 0, 4 };
            Compiler::PrintValue(&buffer, buffer_info, v);
            Log::Print(Log::LOG_INFO, buffer);
            free(buffer);

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
            if (BytecodeObjectManager::ValueFalsey(Peek(0)))
                frame->IP += offset;
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
            int isEnd = ReadByte(frame);
            Sint32 offset = ReadSInt16(frame);

            if (isEnd == 0) {
                VMValue receiver = Peek(0);
                if (receiver.Type == VAL_NULL) {
                    frame->IP += offset;
                    Pop(); // pop receiver
                }
                else if (IS_STRING(receiver)) {
                    // iterate through objectlist
                    ObjectList* objectList = Scene::ObjectRegistries->Get(AS_CSTRING(receiver));
                    if (!objectList)
                        objectList = Scene::ObjectLists->Get(AS_CSTRING(receiver));

                    Pop(); // pop receiver

                    if (!objectList) {
                        // ThrowError(false, "Cannot find object class of name \"%s\".", AS_CSTRING(receiver));
                        frame->IP += offset;
                        break;
                    }

                    if (objectList->Count() == 0) {
                        frame->IP += offset;
                        break;
                    }

                    // Add iterator
                    if (objectList->Registry)
                        *WithIteratorStackTop = WithIter { NULL, NULL, 0, objectList };
                    else
                        *WithIteratorStackTop = WithIter { objectList->EntityFirst, objectList->EntityFirst->NextEntityInList, 0, objectList };
                    WithIteratorStackTop++;

                    BytecodeObject* object = (BytecodeObject*)objectList->EntityFirst;
                    if (objectList->Registry)
                        object = (BytecodeObject*)objectList->GetNth(0);

                    // Backup original receiver
                    BytecodeObjectManager::Globals->Put("other", frame->Slots[0]);
                    *WithReceiverStackTop = frame->Slots[0];
                    WithReceiverStackTop++;
                    // Replace receiver
                    frame->Slots[0] = OBJECT_VAL(object->Instance);
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
                }
            }
            else {
                WithReceiverStackTop--;
                VMValue originalReceiver = *WithReceiverStackTop;
                // Restore original receiver
                frame->Slots[0] = originalReceiver;

                WithIteratorStackTop--;
                WithIter it = *WithIteratorStackTop;

                if (it.list) {
                    it.entity = it.entityNext;
                    if (!((ObjectList*)it.list)->Registry && it.entity) {
                        it.entityNext = ((Entity*)it.entity)->NextEntityInList;

                        frame->IP -= offset;

                        // Put iterator back onto stack
                        *WithIteratorStackTop = it;
                        WithIteratorStackTop++;

                        BytecodeObject* object = (BytecodeObject*)it.entity;
                        // Backup original receiver
                        *WithReceiverStackTop = originalReceiver;
                        WithReceiverStackTop++;
                        // Replace receiver
                        frame->Slots[0] = OBJECT_VAL(object->Instance);
                    }
                    else if (((ObjectList*)it.list)->Registry && ++it.index < ((ObjectList*)it.list)->Count()) {
                        frame->IP -= offset;

                        // Put iterator back onto stack
                        *WithIteratorStackTop = it;
                        WithIteratorStackTop++;

                        BytecodeObject* object = (BytecodeObject*)((ObjectList*)it.list)->GetNth(it.index);
                        // Backup original receiver
                        *WithReceiverStackTop = originalReceiver;
                        WithReceiverStackTop++;
                        // Replace receiver
                        frame->Slots[0] = OBJECT_VAL(object->Instance);
                    }
                    else {
                        // If we are done
                    }
                }
                else {
                    printf("hey you might need to handle the stack here\n");
                    PrintStack();
                    exit(0);
                }
            }
            break;
        }
        case OP_CALL: {
            int argCount = ReadByte(frame);
            if (!CallValue(Peek(argCount), argCount)) {
                ThrowError(true, "Could not call value!");
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &Frames[FrameCount - 1];
            break;
        }
        case OP_INVOKE: {
            Uint32 argCount = ReadByte(frame);
            Uint32 hash = ReadUInt32(frame);

            // char*  hashName = NULL;
            // if (__Tokens__ && __Tokens__->Exists(hash))
            //     hashName = __Tokens__->Get(hash);

            VMValue receiver = Peek(argCount);

            if (IS_CLASS(receiver)) {
                ObjClass* klass = AS_CLASS(receiver);
                if (klass->Methods->Exists(hash)) {
                    VMValue nat = klass->Methods->Get(hash);
                    if (IS_NATIVE(nat)) {
                        if (!CallValue(nat, argCount)) {
                            ThrowError(true, "Could not call value!", hash);
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
                        // ThrowError(true, "Cannot get non-native function value from class.");
                        // return INTERPRET_RUNTIME_ERROR;
                        if (!CallValue(nat, argCount)) {
                            ThrowError(true, "Could not call value!", hash);
                            return INTERPRET_RUNTIME_ERROR;
                        }
                    }
                }
                else {
                    if (__Tokens__ && __Tokens__->Exists(hash))
                        ThrowError(true, "Event \"%s\" does not exist in class %s.", __Tokens__->Get(hash), __Tokens__->Get(klass->Hash));

                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

            if (!Invoke(hash, argCount)) {
                if (__Tokens__ && __Tokens__->Exists(hash))
                    ThrowError(true, "Could not invoke %s value!", __Tokens__->Get(hash));
                return INTERPRET_RUNTIME_ERROR;
            }

            frame = &Frames[FrameCount - 1];
            break;
        }
        case OP_CLASS: {
            Uint32 hash = ReadUInt32(frame);
            ObjClass* klass = NewClass(hash);

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
                printf("ret: %d!!!!!!!!!!!!\n", ret);
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
                ThrowError(false, "Undefined property $[%08X].", hash);
            else
                ThrowError(false, "Undefined property \"%s\".", __Tokens__->Get(hash));
            Pop(); // Instance.
            Push(NULL_VAL);
            return false;
        }

        method = klass->Methods->Get(hash);

        ObjBoundMethod* bound = NewBoundMethod(Peek(0), AS_FUNCTION(method));
        Pop(); // Instance.
        Push(OBJECT_VAL(bound));
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
                        ThrowError(true, "Expected no arguments to initializer, got %d.", argCount);
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

                    VMValue result = native(argCount, StackTop - argCount, ID);
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
        if (ThrowError(true, "Expected %d arguments to function call, got %d.", function->Arity, argCount))
            return false;
    }

    // if (__Tokens__ && __Tokens__->Exists(function->NameHash))
    //     printf("new frame '%s' at %d#%d\n", __Tokens__->Get(function->NameHash), ID, FrameCount);

    if (FrameCount == FRAMES_MAX) {
        ThrowError(true, "Frame overflow. (Count %d / %d)", FrameCount, FRAMES_MAX);
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
            //     ThrowError(true, "Undefined property $[%08X].", hash);
            // else
            //     ThrowError(true, "Undefined property \"%s\".", __Tokens__->Get(hash));
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
        ThrowError(true, "Only instances have methods.");
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
const char* BasicTypesNames[6] = {
    "Null",
    "Integer",
    "Decimal",
    "Object",
    "LinkedInteger",
    "LinkedDecimal",
};
const char* ObjectTypesNames[8] = {
    "Bound Method",
    "Class",
    "Closure",
    "Function",
    "Instance",
    "Native",
    "String",
    "Upvalue",
};

#define IS_NOT_NUMBER(a) (a.Type != VAL_DECIMAL && a.Type != VAL_INTEGER && a.Type != VAL_LINKED_DECIMAL && a.Type != VAL_LINKED_INTEGER)
#define CHECK_IS_NUM(a, b) if (IS_NOT_NUMBER(a)) ThrowError(true, "Cannot perform %s operation on non-number value of type %s.", #b, a.Type == VAL_OBJECT ? ObjectTypesNames[OBJECT_TYPE(a)] : BasicTypesNames[a.Type]);

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
        return DECIMAL_VAL(a_d / b_d);
    }
    int a_d = AS_INTEGER(a);
    int b_d = AS_INTEGER(b);
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
    if (a.Type == VAL_DECIMAL) {
        float a_d = AS_DECIMAL(a);
        return DECIMAL_VAL(++a_d);
    }
    int a_d = AS_INTEGER(a);
    return INTEGER_VAL(++a_d);
}
PUBLIC VMValue VMThread::Values_Decrement() {
    VMValue a = Pop();
    if (a.Type == VAL_DECIMAL) {
        float a_d = AS_DECIMAL(a);
        return DECIMAL_VAL(--a_d);
    }
    int a_d = AS_INTEGER(a);
    return INTEGER_VAL(--a_d);
}
PUBLIC VMValue VMThread::Values_Negate() {
    VMValue a = Pop();
    if (a.Type == VAL_DECIMAL) {
        return DECIMAL_VAL(-AS_DECIMAL(a));
    }
    return INTEGER_VAL(-AS_INTEGER(a));
}
PUBLIC VMValue VMThread::Values_LogicalNOT() {
    VMValue a = Pop();

    // HACK: Yikes.
    if (a.Type == VAL_NULL) {
        return INTEGER_VAL(true);
    }
    else if (a.Type == VAL_OBJECT) {
        return INTEGER_VAL(false);
    }
    else if (a.Type == VAL_DECIMAL) {
        return DECIMAL_VAL((float)(AS_DECIMAL(a) == 0.0));
    }

    return INTEGER_VAL(!AS_INTEGER(a));
}
PUBLIC VMValue VMThread::Values_BitwiseNOT() {
    VMValue a = Pop();
    if (a.Type == VAL_DECIMAL) {
        return DECIMAL_VAL((float)(~(int)AS_DECIMAL(a)));
    }
    return INTEGER_VAL(~AS_INTEGER(a));
}
// #endregion
