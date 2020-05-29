#ifndef ENGINE_RESOURCETYPES_ISPRITE_H
#define ENGINE_RESOURCETYPES_ISPRITE_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED

class Texture;

#include <Engine/Includes/Standard.h>
#include <Engine/Sprites/Animation.h>
#include <Engine/Rendering/Texture.h>

class ISprite {
public:
    char              Filename[256];
    bool              Print = false;
    Texture*          Spritesheets[4];
    bool              SpritesheetsBorrowed[4];
    char              SpritesheetsFilenames[128][4];
    int               SpritesheetCount = 0;
    int               CollisionBoxCount = 0;
    vector<Animation> Animations;

    ISprite();
    ISprite(const char* filename);
    static Texture* AddSpriteSheet(const char* filename);
    void ReserveAnimationCount(int count);
    void AddAnimation(const char* name, int animationSpeed, int frameToLoop);
    void AddAnimation(const char* name, int animationSpeed, int frameToLoop, int frmAlloc);
    void AddFrame(int duration, int left, int top, int width, int height, int pivotX, int pivotY);
    void AddFrame(int duration, int left, int top, int width, int height, int pivotX, int pivotY, int id);
    bool LoadAnimation(const char* filename);
    int  FindAnimation(const char* animname);
    void LinkAnimation(vector<Animation> ani);
    bool SaveAnimation(const char* filename);
    void Dispose();
    ~ISprite();
};

#endif /* ENGINE_RESOURCETYPES_ISPRITE_H */
