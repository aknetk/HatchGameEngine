#if INTERFACE
#include <Engine/Includes/Standard.h>

class RingBuffer {
public:
    int   Size;
    int   Length;
    int   WritePos;
    int   ReadPos;
    char* Data;
};
#endif

#include <Engine/Media/Utils/RingBuffer.h>

#include <Engine/Diagnostics/Log.h>

PUBLIC        RingBuffer::RingBuffer(Uint32 size) {
    this->Length = 0;
    this->WritePos = 0;
    this->ReadPos = 0;

    this->Size = size;
    this->Data = (char*)malloc(this->Size);
    if (this->Data == NULL) {
        Log::Print(Log::LOG_ERROR, "Something went horribly wrong. (Ran out of memory at RingBuffer::RingBuffer)");
        exit(-1);
    }
}
PUBLIC        RingBuffer::~RingBuffer() {
    free(this->Data);
}
PUBLIC int    RingBuffer::Write(const char* data, int len) {
    int k;
    len = (len > (this->Size - this->Length)) ? (this->Size - this->Length) : len;
    if (this->Length < this->Size) {
        if (len + this->WritePos > this->Size) {
            k = (len + this->WritePos) % this->Size;
            memcpy((this->Data + this->WritePos), data, len - k);
            memcpy(this->Data, data + (len - k), k);
        }
        else {
            memcpy((this->Data + this->WritePos), data, len);
        }
        this->Length += len;
        this->WritePos += len;
        if (this->WritePos >= this->Size) {
            this->WritePos = this->WritePos % this->Size;
        }
        return len;
    }
    return 0;
}

PUBLIC void   RingBuffer::ReadData(char* data, const int len) {
    int k;
    if (len + this->ReadPos > this->Size) {
        k = (len + this->ReadPos) % this->Size;
        memcpy(data, this->Data + this->ReadPos, len - k);
        memcpy(data + (len - k), this->Data, k);
    }
    else {
        memcpy(data, this->Data + this->ReadPos, len);
    }
}
PUBLIC int    RingBuffer::Read(char* data, int len) {
    len = (len > this->Length) ? this->Length : len;
    if (this->Length > 0) {
        ReadData(data, len);
        this->Length -= len;
        this->ReadPos += len;
        if (this->ReadPos >= this->Size) {
            this->ReadPos = this->ReadPos % this->Size;
        }
        return len;
    }
    return 0;
}
PUBLIC int    RingBuffer::Peek(char *data, int len) {
    len = (len > this->Length) ? this->Length : len;
    if (this->Length > 0) {
        ReadData(data, len);
        return len;
    }
    return 0;
}
PUBLIC int    RingBuffer::Advance(int len) {
    len = (len > this->Length) ? this->Length : len;
    if (this->Length > 0) {
        this->Length -= len;
        this->ReadPos += len;
        if (this->ReadPos >= this->Size) {
            this->ReadPos = this->ReadPos % this->Size;
        }
        return len;
    }
    return 0;
}
PUBLIC int    RingBuffer::GetLength() {
    return this->Length;
}
PUBLIC int    RingBuffer::GetSize() {
    return this->Size;
}
PUBLIC int    RingBuffer::GetFree() {
    return this->Size - this->Length;
}
