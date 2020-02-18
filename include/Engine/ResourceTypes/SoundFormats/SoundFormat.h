#ifndef SOUNDFORMAT_H
#define SOUNDFORMAT_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL

class Stream;

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

    virtual int    LoadSamples(int count);
    virtual int    GetSamples(Uint8* buffer, int count);
    virtual int    SeekSample(int index);
    virtual int    TellSample();
    virtual double GetPosition();
    virtual double SetPosition(double seconds);
    virtual double GetDuration();
    virtual        ~SoundFormat();
    virtual void   Close();
    virtual void   Dispose();
protected:
    void        LoadFinish();
};

#endif /* SOUNDFORMAT_H */
