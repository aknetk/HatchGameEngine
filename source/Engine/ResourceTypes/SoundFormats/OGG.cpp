#if INTERFACE
#include <Engine/Includes/Standard.h>
#include <Engine/Includes/StandardSDL2.h>

#include <Engine/ResourceTypes/SoundFormats/SoundFormat.h>

class OGG : public SoundFormat {
private:
    // OGG Specific
    char           Vorbis[0x800];
};
#endif

#include <Engine/Application.h>

#include <Engine/Diagnostics/Log.h>
#include <Engine/Diagnostics/Memory.h>
#include <Engine/IO/ResourceStream.h>
#include <Engine/ResourceTypes/SoundFormats/OGG.h>

#define OV_EXCLUDE_STATIC_CALLBACKS
#if defined(OGG_HEADER)
    #include OGG_HEADER
#elif defined(OGG_USE_TREMOR)
    #include <tremor/ivorbisfile.h>
#else
    #include <vorbis/vorbisfile.h>
#endif


#ifdef USING_LIBOGG
struct VorbisGroup {
	OggVorbis_File File;
	vorbis_info*   Info = NULL;
	int            Bitstream;
};
#endif

PRIVATE STATIC size_t      OGG::StaticRead(void* mem, size_t size, size_t nmemb, void* ptr) {
	class Stream* stream = (class Stream*)ptr;
	return stream->ReadBytes(mem, size * nmemb);
}
PRIVATE STATIC Sint32      OGG::StaticSeek(void* ptr, Sint64 offset, int whence) {
	class Stream* stream = (class Stream*)ptr;
    if (whence == SEEK_CUR) {
		stream->Skip((Sint64)offset);
        return 0;
    }
    else if (whence == SEEK_SET) {
		stream->Seek((Sint64)offset);
        return 0;
    }
    else if (whence == SEEK_END) {
		stream->SeekEnd((Sint64)offset);
        return 0;
    }
    else {
        printf("OGG::StaticSeek: %d\n", whence);
        return -1;
    }
}
PRIVATE STATIC Sint32      OGG::StaticCloseFree(void* ptr) {
    return 0;
}
PRIVATE STATIC Sint32      OGG::StaticCloseNoFree(void* ptr) {
    return 0;
}
PRIVATE STATIC long        OGG::StaticTell(void* ptr) {
    return ((class Stream*)ptr)->Position();
}

PUBLIC STATIC SoundFormat* OGG::Load(const char* filename) {
#ifdef USING_LIBOGG
    VorbisGroup* vorbis;
#endif
    OGG* ogg = NULL;
    class Stream* stream = ResourceStream::New(filename);
    if (!stream) {
        Log::Print(Log::LOG_ERROR, "Could not open file '%s'!", filename);
        goto OGG_Load_FAIL;
    }

#ifdef USING_LIBOGG

    ogg = new OGG;
    ogg->StreamPtr = stream;

    ov_callbacks callbacks;
	// callbacks = OV_CALLBACKS_STREAMONLY;
	// callbacks = OV_CALLBACKS_STREAMONLY_NOCLOSE;
	// callbacks = OV_CALLBACKS_DEFAULT;
	// callbacks = OV_CALLBACKS_NOCLOSE;

    callbacks.read_func = OGG::StaticRead;
    callbacks.seek_func = OGG::StaticSeek;
    callbacks.tell_func = OGG::StaticTell;
    callbacks.close_func = OGG::StaticCloseNoFree;

    vorbis = (VorbisGroup*)ogg->Vorbis;

    memset(&vorbis->File, 0, sizeof(OggVorbis_File));

    int res;
    if ((res = ov_open_callbacks(ogg->StreamPtr, &vorbis->File, NULL, 0, callbacks)) != 0) {
        Log::Print(Log::LOG_ERROR, "Could not read file '%s'!", filename);
        switch (res) {
            case OV_EREAD:
                Log::Print(Log::LOG_ERROR, "A read from media returned an error.");
                break;
            case OV_ENOTVORBIS:
                Log::Print(Log::LOG_ERROR, "Bitstream does not contain any Vorbis data.");
                break;
            case OV_EVERSION:
                Log::Print(Log::LOG_ERROR, "Vorbis version mismatch.");
                break;
            case OV_EBADHEADER:
                Log::Print(Log::LOG_ERROR, "Invalid Vorbis bitstream header.");
                break;
            case OV_EFAULT:
                Log::Print(Log::LOG_ERROR, "Internal logic fault; indicates a bug or heap/stack corruption.");
                break;
            default:
                Log::Print(Log::LOG_ERROR, "Resource is not valid Vorbis stream!");
                break;
        }
        goto OGG_Load_FAIL;
    }

    vorbis->Info = ov_info(&vorbis->File, -1);

    memset(&ogg->InputFormat, 0, sizeof(SDL_AudioSpec));
    ogg->InputFormat.format    = AUDIO_S16;
    ogg->InputFormat.channels  = vorbis->Info->channels;
    ogg->InputFormat.freq      = (int)vorbis->Info->rate;
    ogg->InputFormat.samples   = 4096;

    ogg->TotalPossibleSamples = (int)ov_pcm_total(&vorbis->File, -1);

    // Common
    ogg->LoadFinish();

    goto OGG_Load_SUCCESS;

#endif

    OGG_Load_FAIL:
    if (ogg)
        ogg->Dispose();
    delete ogg;
    ogg = NULL;

    OGG_Load_SUCCESS:
    return ogg;
}

PUBLIC        int          OGG::LoadSamples(size_t count) {
#ifdef USING_LIBOGG
    int read;
    Uint32 total = 0,
           bytesForSample = 0;

    if (count > TotalPossibleSamples - Samples.size())
        count = TotalPossibleSamples - Samples.size();

    size_t remainingBytes = count * SampleSize;
    size_t sampleSizeForOneChannel = SampleSize / InputFormat.channels;

    char* buffer = (char*)SampleBuffer + Samples.size() * SampleSize;
    char* bufferStartSample = buffer;

    VorbisGroup* vorbis = (VorbisGroup*)this->Vorbis;

    while (remainingBytes && (read =
		#ifdef OGG_USE_TREMOR
	    ov_read(&vorbis->File, buffer, remainingBytes, &vorbis->Bitstream)
		#else
		ov_read(&vorbis->File, buffer, remainingBytes, 0, sampleSizeForOneChannel, 1, &vorbis->Bitstream)
		#endif
		)) {
        #ifdef DEBUG
            if (read == OV_HOLE)
                return 0;
            if (read == OV_EBADLINK)
                return 0;
            if (read == OV_EINVAL)
                return 0;
        #endif

        remainingBytes -= read;
        buffer += read;
        total += read;

        bytesForSample += read;
        while (bytesForSample >= SampleSize) {
            Samples.push_back((Uint8*)bufferStartSample);
            bufferStartSample += SampleSize;
            bytesForSample -= SampleSize;
        }
    }
    total /= SampleSize;
    return total;
#else
    return 0;
#endif
}

PUBLIC        void         OGG::Dispose() {
#ifdef USING_LIBOGG
	// OGG specific clean up functions
	VorbisGroup* vorbis = (VorbisGroup*)this->Vorbis;
	ov_clear(&vorbis->File);
#endif
    // Common cleanup
    SoundFormat::Dispose();
}
