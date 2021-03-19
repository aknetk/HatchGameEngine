#if INTERFACE
#include <Engine/Includes/Standard.h>
#include <Engine/Includes/StandardSDL2.h>
#include <Engine/IO/Stream.h>
class SDLStream : public Stream {
public:
    SDL_RWops* f;
    enum {
        READ_ACCESS = 0,
        WRITE_ACCESS = 1,
        APPEND_ACCESS = 2,
    };
};
#endif

#include <Engine/IO/SDLStream.h>

PUBLIC STATIC SDLStream* SDLStream::New(const char* filename, Uint32 access) {
    SDLStream* stream = new SDLStream;
    if (!stream) {
        return NULL;
    }

    const char* accessString = NULL;

    switch (access) {
        case SDLStream::READ_ACCESS: accessString = "rb"; break;
        case SDLStream::WRITE_ACCESS: accessString = "wb"; break;
        case SDLStream::APPEND_ACCESS: accessString = "ab"; break;
    }

    stream->f = SDL_RWFromFile(filename, accessString);
    if (!stream->f)
        goto FREE;

    return stream;

    FREE:
        delete stream;
        return NULL;
}

PUBLIC        void        SDLStream::Close() {
    SDL_RWclose(f);
    f = NULL;
    Stream::Close();
}
PUBLIC        void        SDLStream::Seek(Sint64 offset) {
    SDL_RWseek(f, offset, RW_SEEK_SET);
}
PUBLIC        void        SDLStream::SeekEnd(Sint64 offset) {
    SDL_RWseek(f, offset, RW_SEEK_END);
}
PUBLIC        void        SDLStream::Skip(Sint64 offset) {
    SDL_RWseek(f, offset, RW_SEEK_CUR);
}
PUBLIC        size_t      SDLStream::Position() {
    return SDL_RWtell(f);
}
PUBLIC        size_t      SDLStream::Length() {
    return SDL_RWsize(f);
}

PUBLIC        size_t      SDLStream::ReadBytes(void* data, size_t n) {
    // if (!f) Log::Print(Log::LOG_ERROR, "Attempt to read from closed stream.")
    return SDL_RWread(f, data, 1, n);
}

PUBLIC        size_t      SDLStream::WriteBytes(void* data, size_t n) {
    return SDL_RWwrite(f, data, 1, n);
}
