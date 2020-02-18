#ifndef WAV_H
#define WAV_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Includes/Standard.h>
#include <Engine/Includes/StandardSDL2.h>
#include <Engine/ResourceTypes/SoundFormats/SoundFormat.h>

class WAV : public SoundFormat {
private:
    // WAV Specific
    int DataStart = 0;


public:
    static SoundFormat* Load(const char* filename);
           int          LoadSamples(int count);
           void         Dispose();
};

#endif /* WAV_H */
