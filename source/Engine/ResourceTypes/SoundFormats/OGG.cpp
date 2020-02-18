#if INTERFACE
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
};
#endif

#include <Engine/Application.h>

#include <Engine/Diagnostics/Log.h>
#include <Engine/Diagnostics/Memory.h>
#include <Engine/IO/ResourceStream.h>
#include <Engine/ResourceTypes/SoundFormats/OGG.h>

PRIVATE STATIC size_t      OGG::StaticRead(void* mem, size_t size, size_t nmemb, void* ptr) {
	class Stream* stream = (class Stream*)ptr;
	return stream->ReadBytes(mem, size * nmemb);
}
PRIVATE STATIC Sint32      OGG::StaticSeek(void* ptr, ogg_int64_t offset, int whence) {
	class Stream* stream = (class Stream*)ptr;
    if (whence == SEEK_CUR) {
		stream->Skip((Uint64)offset);
        return 0;
    }
    else if (whence == SEEK_SET) {
		stream->Seek((Uint64)offset);
        return 0;
    }
    else if (whence == SEEK_END) {
		stream->SeekEnd((Uint64)offset);
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
    OGG* ogg = NULL;
    class Stream* stream = ResourceStream::New(filename);
    if (!stream) {
        Log::Print(Log::LOG_ERROR, "Could not open file '%s'!", filename);
        goto OGG_Load_FAIL;
    }

    ogg = new OGG;
    ogg->StreamPtr = stream;

    ov_callbacks callbacks;
	callbacks = OV_CALLBACKS_STREAMONLY;
	callbacks = OV_CALLBACKS_STREAMONLY_NOCLOSE;
	callbacks = OV_CALLBACKS_DEFAULT;
	callbacks = OV_CALLBACKS_NOCLOSE;

    callbacks.read_func = OGG::StaticRead;
    callbacks.seek_func = OGG::StaticSeek;
    callbacks.tell_func = OGG::StaticTell;

    memset(&ogg->VorbisFile, 0, sizeof(OggVorbis_File));
    if (ov_open_callbacks(ogg->StreamPtr, &ogg->VorbisFile, NULL, 0, callbacks) != 0) {
        Log::Print(Log::LOG_ERROR, "Resource is not valid Vorbis stream!");
        goto OGG_Load_FAIL;
    }

    ogg->VorbisInfo = ov_info(&ogg->VorbisFile, -1);

    memset(&ogg->InputFormat, 0, sizeof(SDL_AudioSpec));
    ogg->InputFormat.format    = AUDIO_S16;
    ogg->InputFormat.channels  = ogg->VorbisInfo->channels;
    ogg->InputFormat.freq      = ogg->VorbisInfo->rate;
    ogg->InputFormat.samples   = 4096;

    ogg->TotalPossibleSamples = (int)ov_pcm_total(&ogg->VorbisFile, -1);

    // Common
    ogg->LoadFinish();

    goto OGG_Load_SUCCESS;

    OGG_Load_FAIL:
    ogg->Dispose();
    delete ogg;
    ogg = NULL;

    OGG_Load_SUCCESS:
    return ogg;
}

PUBLIC        int          OGG::LoadSamples(int count) {
    int read,
        total = 0,
        bytesForSample = 0;

    if ((size_t)count > TotalPossibleSamples - Samples.size())
        count = TotalPossibleSamples - Samples.size();

    int remainingBytes = count * SampleSize;
    int sampleSizeForOneChannel = SampleSize / InputFormat.channels;

    char* buffer = (char*)SampleBuffer + Samples.size() * SampleSize;
    char* bufferStartSample = buffer;

    while (remainingBytes && (read = ov_read(&VorbisFile, buffer, remainingBytes, 0, sampleSizeForOneChannel, 1, &VorbisBitstream))) {
        #ifdef DEBUG
            if (read == OV_HOLE) {
                return 0;
            }
            if (read == OV_EBADLINK) {
                return 0;
            }
            if (read == OV_EINVAL) {
                return 0;
            }
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
}

PUBLIC        void         OGG::Dispose() {
	// BUG: If we try to free rockon.ogg after it's done playing.
    // OGG specific clean up functions
    ov_clear(&VorbisFile);
    // Common cleanup
    SoundFormat::Dispose();
}
