#if INTERFACE
#include <Engine/Includes/Standard.h>
#include <Engine/Includes/StandardSDL2.h>
#include <Engine/Application.h>
#include <Engine/Audio/StackNode.h>
#include <Engine/ResourceTypes/ISound.h>

class AudioManager {
public:
    static SDL_AudioDeviceID Device;
    static SDL_AudioSpec     DeviceFormat;
    static bool              AudioEnabled;

    static deque<StackNode*> MusicStack;
    static StackNode*        SoundArray;
    static int               SoundArrayLength;

    static double            FadeOutTimer;
    static double            FadeOutTimerMax;

    static float             MasterVolume;
    static float             MusicVolume;
    static float             SoundVolume;

    static float             LowPassFilter;

    static Uint8*            AudioQueue;
    static size_t            AudioQueueSize;
    static size_t            AudioQueueMaxSize;

    enum {
        REQUEST_EOF = 0,
        REQUEST_ERROR = -1,
        REQUEST_CONVERTING = -2,
    };
};
#endif

#include <Engine/Audio/AudioManager.h>
#include <Engine/Diagnostics/Log.h>
#include <Engine/Diagnostics/Memory.h>

SDL_AudioDeviceID AudioManager::Device;
SDL_AudioSpec     AudioManager::DeviceFormat;
bool              AudioManager::AudioEnabled = false;

deque<StackNode*> AudioManager::MusicStack;
StackNode*        AudioManager::SoundArray = NULL;
int               AudioManager::SoundArrayLength = 512;

double            AudioManager::FadeOutTimer = 1.0;
double            AudioManager::FadeOutTimerMax = 1.0;

float             AudioManager::MasterVolume = 1.0f;
float             AudioManager::MusicVolume = 1.0f;
float             AudioManager::SoundVolume = 1.0f;

float             AudioManager::LowPassFilter = 0.0f;

Uint8*            AudioManager::AudioQueue = NULL;
size_t            AudioManager::AudioQueueSize = 0;
size_t            AudioManager::AudioQueueMaxSize = 0;

enum {
    FILTER_TYPE_LOW_PASS,
    FILTER_TYPE_HIGH_PASS,
};
double a0 = 0.0;
double a1 = 0.0;
double a2 = 0.0;
double b1 = 0.0;
double b2 = 0.0;
double mQuality = 1.0;
double mGain = 1.0;
double mNormalizedFreq = 1.0 / 64.0; // Good for low pass
int    mFilterType = FILTER_TYPE_LOW_PASS;
// double mNormalizedFreq = 1.0 / 8.0;
// int    mFilterType = FILTER_TYPE_HIGH_PASS;
Sint16 mZx[2 * 2]; // 2 per channel
Sint16 mZy[2 * 2]; // 2 per channel
float  mZxF[2 * 2]; // 2 per channel
float  mZyF[2 * 2]; // 2 per channel

PUBLIC STATIC void   AudioManager::CalculateCoeffs() {
    double theta = 2.0 * M_PI * mNormalizedFreq; // normalized frequency has been precalculated as fc/fs
    double d = 0.5 * (1.0 / mQuality) * sin(theta);
    double beta = 0.5 * ( (1.0 - d) / (1.0 + d) );
    double gamma = (0.5 + beta) * cos(theta);

    mZx[0] = 0;
    mZx[1] = 0;
    mZy[0] = 0;
    mZy[1] = 0;

    mZxF[0] = 0;
    mZxF[1] = 0;
    mZyF[0] = 0;
    mZyF[1] = 0;

    if (mFilterType == FILTER_TYPE_LOW_PASS) {
        a0 = 0.5 * (0.5 + beta - gamma);
        a1 = 0.5 + beta - gamma;
    }
    else {
        a0 = 0.5 * (0.5 + beta + gamma);
        a1 = -(0.5 + beta + gamma);
    }

    a2 = a0;
    b1 = -2.0 * gamma;
    b2 = 2.0 * beta;
}
PUBLIC STATIC Sint16 AudioManager::ProcessSample(Sint16 inSample, int channel) {
    Sint16 outSample;
    int idx0 = 2 * channel;
    int idx1 = idx0 + 1;

    outSample = (Sint16)(
        (a0 * inSample)  + (a1 * mZx[idx0]) +
        (a2 * mZx[idx1]) - (b1 * mZy[idx0]) - (b2 * mZy[idx1]));

    // if (outSample > 0x3FFF)
    //     outSample = 0x3FFF;
    // if (outSample < -0x3FFF)
    //     outSample = -0x3FFF;

    mZx[idx1] = mZx[idx0];
    mZx[idx0] = inSample;
    mZy[idx1] = mZy[idx0];
    mZy[idx0] = outSample;

    return (Sint16)(inSample * (1.0 - AudioManager::LowPassFilter) + outSample * mGain * AudioManager::LowPassFilter);
}
PUBLIC STATIC float  AudioManager::ProcessSampleFloat(float inSample, int channel) {
    float outSample;
    int idx0 = 2 * channel;
    int idx1 = idx0 + 1;

    outSample = (float)(
        (a0 * inSample)  + (a1 * mZxF[idx0]) +
        (a2 * mZxF[idx1]) - (b1 * mZyF[idx0]) - (b2 * mZyF[idx1]));

    mZxF[idx1] = mZxF[idx0];
    mZxF[idx0] = inSample;
    mZyF[idx1] = mZyF[idx0];
    mZyF[idx0] = outSample;

    return (float)(inSample * (1.0 - AudioManager::LowPassFilter) + outSample * mGain * AudioManager::LowPassFilter);
}

PUBLIC STATIC void   AudioManager::Init() {
    CalculateCoeffs();

    SoundArray = (StackNode*)Memory::Calloc(SoundArrayLength, sizeof(StackNode));
    for (int i = 0; i < SoundArrayLength; i++)
        SoundArray[i].Paused = true;

    SDL_AudioSpec Want;
    memset(&Want, 0, sizeof(Want));

    Want.freq = 44100;
    Want.format = AUDIO_S16;
    Want.samples = 0x800;
    Want.channels = 2;
    Want.callback = AudioManager::AudioCallback;

    // Lower sample mix rate for weaker devices
    switch (Application::Platform) {
        case Platforms::Android:
            Want.samples = 0x200;
            break;
        default:
            break;
    }

    if (Application::Platform != Platforms::Android) {
        if (SDL_OpenAudio(&Want, &DeviceFormat) >= 0) {
            AudioEnabled = true;
            SDL_PauseAudio(0);
            Device = 1;
        }
        else Log::Print(Log::LOG_ERROR, "Could not open audio device!");
    }
    else {
        if ((Device = SDL_OpenAudioDevice(NULL, 0, &Want, &DeviceFormat, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE))) {
            AudioEnabled = true;
            SDL_PauseAudioDevice(Device, 0);
        }
        else Log::Print(Log::LOG_ERROR, "Could not open audio device!");
    }

    Log::Print(Log::LOG_INFO, "%s", "Audio Device:");
    Log::Print(Log::LOG_INFO, "%s = %d", "Freq", DeviceFormat.freq);
    Log::Print(Log::LOG_INFO, "%s = %s%d-bit%s", "Format",
        SDL_AUDIO_ISSIGNED(DeviceFormat.format) ? "signed " : "unsigned ",
        SDL_AUDIO_BITSIZE(DeviceFormat.format),
        SDL_AUDIO_ISFLOAT(DeviceFormat.format) ? " (float)" : SDL_AUDIO_ISBIGENDIAN(DeviceFormat.format) ? " BE" : " LE");
    Log::Print(Log::LOG_INFO, "%s = %X", "Samples", DeviceFormat.samples);
    Log::Print(Log::LOG_INFO, "%s = %X", "Channels", DeviceFormat.channels);

    // AudioQueue
    AudioQueueMaxSize = DeviceFormat.samples * DeviceFormat.channels * (SDL_AUDIO_BITSIZE(DeviceFormat.format) >> 3);
    // if (AudioQueueMaxSize < 0x1000)
        AudioQueueMaxSize = 0x1000;
    AudioQueueSize    = 0;
    AudioQueue        = (Uint8*)Memory::Calloc(8, AudioQueueMaxSize);
}

PUBLIC STATIC void   AudioManager::SetSound(int channel, ISound* music) {
    AudioManager::SetSound(channel, music, false, 0);
}
PUBLIC STATIC void   AudioManager::SetSound(int channel, ISound* music, bool loop, int loopPoint) {
    AudioManager::Lock();

    StackNode* audio = &SoundArray[channel];

    music->Seek(0);
	if (music->Stream)
		SDL_AudioStreamClear(music->Stream);

    // int infoChannels = music->Format.channels;
    // int infoSampleSize = (music->Format.format & 0xFF) >> 3;

    audio->Audio = music;
    audio->Stopped = false;
    audio->Paused = false;
    audio->Loop = loop;
    audio->LoopPoint = loopPoint;
    audio->FadeOut = false;

    AudioManager::Unlock();
}

PUBLIC STATIC void   AudioManager::PushMusic(ISound* music, bool loop, Uint32 lp) {
    PushMusicAt(music, 0.0, loop, lp);
}
PUBLIC STATIC void   AudioManager::PushMusicAt(ISound* music, double at, bool loop, Uint32 lp) {
    if (music->LoadFailed) return;

    AudioManager::Lock();

    int start_sample = (int)std::ceil(at * music->Format.freq);

    music->Seek(start_sample);
	if (music->Stream)
		SDL_AudioStreamClear(music->Stream);

    StackNode* newms = new StackNode();
    newms->Audio = music;
    newms->Loop = loop;
    newms->LoopPoint = lp;
    newms->FadeOut = false;
    MusicStack.push_front(newms);

    for (size_t i = 1; i < MusicStack.size(); i++)
        MusicStack[i]->FadeOut = false;

    FadeOutTimer = 1.0;
    FadeOutTimerMax = 1.0;

    AudioManager::Unlock();
}
PUBLIC STATIC void   AudioManager::RemoveMusic(ISound* music) {
    AudioManager::Lock();
    for (size_t i = 0; i < MusicStack.size(); i++) {
        if (MusicStack[i]->Audio == music) {
            delete MusicStack[i];
            MusicStack.erase(MusicStack.begin() + i);
        }
    }
    AudioManager::Unlock();
}
PUBLIC STATIC bool   AudioManager::IsPlayingMusic() {
    return MusicStack.size() > 0;
}
PUBLIC STATIC bool   AudioManager::IsPlayingMusic(ISound* music) {
    for (size_t i = 0; i < MusicStack.size(); i++) {
        if (MusicStack[i]->Audio == music) {
            return true;
        }
    }
    return false;
}
PUBLIC STATIC void   AudioManager::ClearMusic() {
    AudioManager::Lock();
    for (size_t i = 0; i < MusicStack.size(); i++) {
        delete MusicStack[i];
    }
    MusicStack.clear();
    AudioManager::Unlock();
}
PUBLIC STATIC void   AudioManager::FadeMusic(double seconds) {
    AudioManager::Lock();
    if (MusicStack.size() > 0) {
        MusicStack[0]->FadeOut = true;
        FadeOutTimer = seconds;
        FadeOutTimerMax = seconds;
    }
    AudioManager::Unlock();
}

PUBLIC STATIC void   AudioManager::Lock() {
    SDL_LockAudioDevice(Device); //// SDL_LockAudio();
}
PUBLIC STATIC void   AudioManager::Unlock() {
    SDL_UnlockAudioDevice(Device); //// SDL_UnlockAudio();
}

PUBLIC STATIC void   AudioManager::AudioUnpause(int channel) {
    AudioManager::Lock();
    SoundArray[channel].Paused = false;
    AudioManager::Unlock();
}
PUBLIC STATIC void   AudioManager::AudioPause(int channel) {
    AudioManager::Lock();
    SoundArray[channel].Paused = true;
    AudioManager::Unlock();
}
PUBLIC STATIC void   AudioManager::AudioStop(int channel) {
    AudioManager::Lock();
    SoundArray[channel].Stopped = true;
    AudioManager::Unlock();
}
PUBLIC STATIC void   AudioManager::AudioUnpauseAll() {
    AudioManager::Lock();
    for (int i = 0; i < SoundArrayLength; i++)
        SoundArray[i].Paused = false;
    AudioManager::Unlock();
}
PUBLIC STATIC void   AudioManager::AudioPauseAll() {
    AudioManager::Lock();
    for (int i = 0; i < SoundArrayLength; i++)
        SoundArray[i].Paused = true;
    AudioManager::Unlock();
}
PUBLIC STATIC void   AudioManager::AudioStopAll() {
    AudioManager::Lock();
    for (int i = 0; i < SoundArrayLength; i++)
        SoundArray[i].Stopped = true;
    AudioManager::Unlock();
}

PUBLIC STATIC bool   AudioManager::AudioPlayMix(StackNode* audio, Uint8* stream, int len, float volume) {
    if (audio->FadeOut) {
        FadeOutTimer -= (double)DeviceFormat.samples / DeviceFormat.freq;
        if (FadeOutTimer < 0.0) {
            FadeOutTimer = 1.0;
            FadeOutTimerMax = 1.0;
            return true;
        }
    }

    int bytes = audio->Audio->RequestSamples(DeviceFormat.samples, audio->Loop, audio->LoopPoint);
    switch (bytes) {
        // End of file
        case REQUEST_EOF:
            return true;
        // Waiting
        case REQUEST_CONVERTING:
            break;
        // Error
        case REQUEST_ERROR:
            break;
        // Normal
        default:
            if (audio->FadeOut)
                SDL_MixAudioFormat(stream, audio->Audio->Buffer, DeviceFormat.format, (Uint32)bytes, (int)(SDL_MIX_MAXVOLUME * MasterVolume * volume * (FadeOutTimer / FadeOutTimerMax)));
            else
                SDL_MixAudioFormat(stream, audio->Audio->Buffer, DeviceFormat.format, (Uint32)bytes, (int)(SDL_MIX_MAXVOLUME * MasterVolume * volume));

            // NOTE: In order to time scale, we need some kind of sample queue system,
            //       and this system needs to intake samples, work a bilinear interpolation
            //       algorithm on them, and then it can work.
    }
    return false;
}

PUBLIC STATIC void   AudioManager::AudioCallback(void* data, Uint8* stream, int len) {
    memset(stream, 0x00, len);

    if (AudioManager::AudioQueueSize >= (size_t)len) {
        // memcpy(stream, AudioManager::AudioQueue, len);
        SDL_MixAudioFormat(stream, AudioManager::AudioQueue, DeviceFormat.format, (Uint32)len, (int)(SDL_MIX_MAXVOLUME * MasterVolume * 1.0f * (FadeOutTimer / FadeOutTimerMax)));

        AudioManager::AudioQueueSize -= len;
        if (AudioManager::AudioQueueSize > 0)
            memmove(AudioManager::AudioQueue, AudioManager::AudioQueue + len, AudioManager::AudioQueueSize);
    }

    // Make track system
    if (MusicStack.size() > 0) {
        StackNode* audio = MusicStack[0];
        if (!audio->Paused)
        if (AudioManager::AudioPlayMix(MusicStack[0], stream, len, MusicVolume)) {
            delete MusicStack[0];
            MusicStack.pop_front();
        }
    }

    for (int i = 0; i < SoundArrayLength; i++) {
        StackNode* audio = &SoundArray[i];
        if (!audio->Audio || audio->Stopped || audio->Paused)
            continue;

        if (AudioManager::AudioPlayMix(audio, stream, len, SoundVolume)) {
            audio->Stopped = true;
        }
    }

    if (LowPassFilter > 0.0) {
        size_t channels = 2;
        if (SDL_AUDIO_ISFLOAT(DeviceFormat.format)) {
            size_t samples = len / sizeof(float) / channels;

            float* in  = (float*)stream;
            float* out = (float*)stream;
            for (size_t i = 0; i < samples; i++) {
                *out++ = ProcessSampleFloat(*in++, 1);
                *out++ = ProcessSampleFloat(*in++, 0);
            }
        }
        else {
            size_t samples = len / sizeof(Sint16) / channels;

            Sint16* in  = (Sint16*)stream;
            Sint16* out = (Sint16*)stream;
            for (size_t i = 0; i < samples; i++) {
                *out++ = (Sint16)ProcessSample(*in++, 1);
                *out++ = (Sint16)ProcessSample(*in++, 0);
            }
        }
    }
}

PUBLIC STATIC void   AudioManager::Dispose() {
    Memory::Free(SoundArray);
    Memory::Free(AudioQueue);

    SDL_PauseAudioDevice(Device, 1); // SDL_PauseAudio(1); //
    SDL_CloseAudioDevice(Device); // SDL_CloseAudio(); //
}
