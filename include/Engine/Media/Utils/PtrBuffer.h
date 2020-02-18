#ifndef PTRBUFFER_H
#define PTRBUFFER_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Includes/Standard.h>
#include <functional>

class PtrBuffer {
public:
    Uint32 ReadPtr;
    Uint32 WritePtr;
    Uint32 Size;
    void** Data;
    void (*FreeFunc)(void*);

    PtrBuffer(Uint32 size, void (*freeFunc)(void*));
    ~PtrBuffer();
    Uint32 GetLength();
    void   Clear();
    void*  Read();
    void*  Peek();
    void   Advance();
    int    Write(void* ptr);
    void   ForEachItemInBuffer(void (*callback)(void*, void*), void* userdata);
    void   WithEachItemInBuffer(std::function<void(void*, void*)> callback, void* userdata);
    int    IsFull();
};

#endif /* PTRBUFFER_H */
