#if INTERFACE
#include <Engine/Includes/Standard.h>
#include <Engine/Includes/StandardSDL2.h>
#include <Engine/IO/Stream.h>

class SoundFormat {
public:
    // Common
    Stream*        StreamPtr = NULL;
    SDL_AudioSpec  InputFormat;
    SDL_AudioSpec  OutputFormat;

    vector<Uint8*> Samples;
    int            SampleSize;
    int            SampleIndex = 0;

    int            TotalPossibleSamples;
    Uint8*         SampleBuffer = NULL;
    Uint8*         SampleBufferHead = NULL;
};
#endif

#include <Engine/ResourceTypes/SoundFormats/SoundFormat.h>

#include <Engine/Application.h>
#include <Engine/Diagnostics/Log.h>
#include <Engine/Diagnostics/Memory.h>

PUBLIC VIRTUAL int    SoundFormat::LoadSamples(int count) {
    return 0;
}
PUBLIC VIRTUAL int    SoundFormat::GetSamples(Uint8* buffer, int count) {
    if ((size_t)SampleIndex >= Samples.size()) {
        if (LoadSamples(count) == 0) // If we've reached end of file
            return 0;
    }

    if (count >= Samples.size() - (size_t)SampleIndex)
        count = Samples.size() - SampleIndex;

    int samplecount = 0;
    for (size_t i = SampleIndex; i < SampleIndex + count && i < Samples.size(); i++) {
        memcpy(buffer, Samples[i], SampleSize);
        buffer += SampleSize;
        samplecount++;
    }
    SampleIndex += samplecount;
    return samplecount;
}
PUBLIC VIRTUAL int    SoundFormat::SeekSample(int index) {
    SampleIndex = index;
    return SampleIndex;
}
PUBLIC VIRTUAL int    SoundFormat::TellSample() {
    return SampleIndex;
}

PUBLIC VIRTUAL double SoundFormat::GetPosition() {
    return (double)SampleIndex / InputFormat.freq;
}
PUBLIC VIRTUAL double SoundFormat::SetPosition(double seconds) {
    SampleIndex = int(seconds * InputFormat.freq);
    return GetPosition();
}
PUBLIC VIRTUAL double SoundFormat::GetDuration() {
    return (double)TotalPossibleSamples / InputFormat.freq;
}

PROTECTED void        SoundFormat::LoadFinish() {
    SampleIndex = 0;
    SampleSize = ((InputFormat.format & 0xFF) >> 3) * InputFormat.channels;
    SampleBuffer = (Uint8*)Memory::TrackedMalloc("SoundData::SampleBuffer", TotalPossibleSamples * SampleSize);
    Samples.reserve(TotalPossibleSamples);
}

PUBLIC VIRTUAL        SoundFormat::~SoundFormat() {
    // Do not add anything to a base class' destructor.
}

PUBLIC VIRTUAL void   SoundFormat::Close() {
    if (StreamPtr) {
        StreamPtr->Close();
        StreamPtr = NULL;
    }
}
PUBLIC VIRTUAL void   SoundFormat::Dispose() {
    Samples.clear();

    if (SampleBuffer)
        Memory::Free(SampleBuffer);

    SampleBufferHead = NULL;

    if (StreamPtr) {
        StreamPtr->Close();
        StreamPtr = NULL;
    }
}
