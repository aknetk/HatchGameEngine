#if INTERFACE
#include <Engine/Includes/Standard.h>
#include <Engine/IO/Stream.h>
class ZLibStream : public Stream {
public:
    Stream*  other_stream = NULL;
    uint32_t mode = 0;

    void*    memory = NULL;
    size_t   memory_size = 0;
};
#endif

#include <Engine/IO/Compression/ZLibStream.h>
#include <Engine/IO/Compression/CompressionEnums.h>

#undef min
#undef max

#include <zlib.h>

PUBLIC STATIC ZLibStream* ZLibStream::New(Stream* other_stream, uint32_t mode) {
    ZLibStream* stream = new ZLibStream;
    if (!stream) {
        return NULL;
    }

    if (!other_stream)
        goto FREE;

    stream->other_stream = other_stream;
    stream->mode = mode;

    if (mode == CompressionMode::DECOMPRESS) {
        size_t inSize = other_stream->Length() - 4;
        void*  inMem = Memory::Malloc(inSize);
        stream->memory_size = other_stream->ReadUInt32BE();
        stream->memory = Memory::Malloc(stream->memory_size);

        other_stream->ReadBytes(inMem, inSize);
        stream->Decompress(inMem, inSize);

        Memory::Free(inMem);
    }

    return stream;

    FREE:
        delete stream;
        return NULL;
}

PUBLIC        void        ZLibStream::Close() {
    if (memory) {
        Memory::Free(memory);
        memory_size = 0;
        memory = NULL;
    }

    Stream::Close();
}
PUBLIC        void        ZLibStream::Seek(Sint64 offset) {
    // pointer = pointer_start + offset;
}
PUBLIC        void        ZLibStream::SeekEnd(Sint64 offset) {
    // pointer = pointer_start + size - offset;
}
PUBLIC        void        ZLibStream::Skip(Sint64 offset) {
    // pointer = pointer + offset;
}
PUBLIC        size_t      ZLibStream::Position() {
    // return pointer - pointer_start;
    return 0;
}
PUBLIC        size_t      ZLibStream::Length() {
    // return pointer - pointer_start;
    return 0;
}

PUBLIC        size_t      ZLibStream::ReadBytes(void* data, size_t n) {
    memcpy(data, memory, n);
    // pointer += n;
    return n;
}

PUBLIC        size_t      ZLibStream::WriteBytes(void* data, size_t n) {
    // memcpy(pointer, data, n);
    // pointer += n;
    return n;
}

PUBLIC STATIC void        ZLibStream::Decompress(void* dst, size_t dstLen, void* src, size_t srcLen) {
    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;

    infstream.next_in = (Bytef*)src;
    infstream.next_out = (Bytef*)dst;
    infstream.avail_in = srcLen;
    infstream.avail_out = dstLen;

    inflateInit(&infstream);
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);
}

PRIVATE       void        ZLibStream::Decompress(void* in, size_t inLen) {
    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;

    infstream.next_in = (Bytef*)in;
    infstream.next_out = (Bytef*)memory;
    infstream.avail_in = inLen;
    infstream.avail_out = memory_size;

    inflateInit(&infstream);
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);
}
