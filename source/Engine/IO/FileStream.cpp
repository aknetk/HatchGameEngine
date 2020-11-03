#if INTERFACE
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
};
#endif

#include <Engine/IO/FileStream.h>
#include <Engine/Includes/StandardSDL2.h>

PUBLIC STATIC FileStream* FileStream::New(const char* filename, Uint32 access) {
    FileStream* stream = new FileStream;
    if (!stream) {
        return NULL;
    }

    const char* accessString = NULL;

    switch (access) {
        case FileStream::READ_ACCESS: accessString = "rb"; break;
        case FileStream::WRITE_ACCESS: accessString = "wb"; break;
        case FileStream::APPEND_ACCESS: accessString = "ab"; break;
    }

    stream->f = fopen(filename, accessString);

    #ifdef SWITCH_ROMFS
    if (!stream->f) {
        char romfsPath[256];
        snprintf(romfsPath, 256, "romfs:/%s", filename);
        stream->f = fopen(romfsPath, accessString);
    }
    #endif

    #ifdef ANDROID
    if (!stream->f) {
        const char* internalStorage = SDL_AndroidGetInternalStoragePath();
        if (internalStorage) {
            char androidPath[256];
            snprintf(androidPath, 256, "%s/%s", internalStorage, filename);
            stream->f = fopen(androidPath, accessString);
        }
    }
    #endif

    if (!stream->f)
        goto FREE;

    fseek(stream->f, 0, SEEK_END);
    stream->size = ftell(stream->f);
    fseek(stream->f, 0, SEEK_SET);

    return stream;

    FREE:
        delete stream;
        return NULL;
}

PUBLIC        void        FileStream::Close() {
    fclose(f);
    f = NULL;
    Stream::Close();
}
PUBLIC        void        FileStream::Seek(Sint64 offset) {
    fseek(f, offset, SEEK_SET);
}
PUBLIC        void        FileStream::SeekEnd(Sint64 offset) {
    fseek(f, offset, SEEK_END);
}
PUBLIC        void        FileStream::Skip(Sint64 offset) {
    fseek(f, offset, SEEK_CUR);
}
PUBLIC        size_t      FileStream::Position() {
    return ftell(f);
}
PUBLIC        size_t      FileStream::Length() {
    return size;
}

PUBLIC        size_t      FileStream::ReadBytes(void* data, size_t n) {
    // if (!f) Log::Print(Log::LOG_ERROR, "Attempt to read from closed stream.")
    return fread(data, 1, n, f);
}

PUBLIC        size_t      FileStream::WriteBytes(void* data, size_t n) {
    return fwrite(data, 1, n, f);
}
