#ifndef FILESTREAM_H
#define FILESTREAM_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Includes/Standard.h>
#include <Engine/IO/Stream.h>

class FileStream : public Stream {
public:
    FILE*  f;
    size_t size;
    enum {
    READ_ACCESS = 0,
    WRITE_ACCESS = 1,
    APPEND_ACCESS = 2,
    }; 

    static FileStream* New(const char* filename, Uint32 access);
           void        Close();
           void        Seek(Sint64 offset);
           void        SeekEnd(Sint64 offset);
           void        Skip(Sint64 offset);
           size_t      Position();
           size_t      Length();
           size_t      ReadBytes(void* data, size_t n);
           size_t      WriteBytes(void* data, size_t n);
};

#endif /* FILESTREAM_H */
