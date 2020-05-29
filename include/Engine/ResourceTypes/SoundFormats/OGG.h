#ifndef ENGINE_RESOURCETYPES_SOUNDFORMATS_OGG_H
#define ENGINE_RESOURCETYPES_SOUNDFORMATS_OGG_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED


#include <Engine/Includes/Standard.h>
#include <Engine/Includes/StandardSDL2.h>
#include <Engine/ResourceTypes/SoundFormats/SoundFormat.h>
#include <vorbis/vorbisfile.h>

class OGG : public SoundFormat {
private:
    // OGG Specific
    OggVorbis_File VorbisFile;
    vorbis_info*   VorbisInfo = NULL;
    int            VorbisBitstream;

    static size_t      StaticRead(void* mem, size_t size, size_t nmemb, void* ptr);
    static Sint32      StaticSeek(void* ptr, ogg_int64_t offset, int whence);
    static Sint32      StaticCloseFree(void* ptr);
    static Sint32      StaticCloseNoFree(void* ptr);
    static long        StaticTell(void* ptr);

public:
    static SoundFormat* Load(const char* filename);
           int          LoadSamples(size_t count);
           void         Dispose();
};

#endif /* ENGINE_RESOURCETYPES_SOUNDFORMATS_OGG_H */
