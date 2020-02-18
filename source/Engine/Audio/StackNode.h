#ifndef ENGINE_AUDIO_STACKNODE_H
#define ENGINE_AUDIO_STACKNODE_H

class ISound;

struct StackNode {
    ISound*  Audio = NULL;
    bool     Loop = false;
    Uint32   LoopPoint = 0;
    bool     FadeOut = false;
    bool     Paused = false;
    bool     Stopped = false;
};

#endif /* ENGINE_AUDIO_STACKNODE_H */
