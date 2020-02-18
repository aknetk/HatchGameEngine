#ifndef ENTITY_H
#define ENTITY_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL

class ObjectList;
class Entity;
class Entity;
class ObjectList;
class Entity;
class Entity;

#include <Engine/Includes/Standard.h>
#include <Engine/Application.h>
#include <Engine/Graphics.h>
#include <Engine/ResourceTypes/ISprite.h>
#include <Engine/Scene.h>
#include <Engine/Types/EntityTypes.h>
#include <Engine/Includes/HashMap.h>

class Entity {
public:
    float        InitialX = 0;
    float        InitialY = 0;
    int          Active = true;
    int          Pauseable = true;
    float        X = 0.0f;
    float        Y = 0.0f;
    float        Z = 0.0f;
    float        XSpeed = 0;
    float        YSpeed = 0;
    int          OnScreen = true;
    float        OnScreenHitboxW = 0.0f;
    float        OnScreenHitboxH = 0.0f;
    int          Angle = 0;
    float        Rotation = 0.0;
    int          Priority = 0;
    int          PriorityListIndex = -1;
    int          PriorityOld = 0;
    int          Sprite = -1;
    int          CurrentAnimation = -1;
    int          CurrentFrame = -1;
    float        CurrentFrameTimeLeft = 0.0;
    float        AnimationSpeedMult = 1.0;
    float        HitboxW = 0.0f;
    float        HitboxH = 0.0f;
    float        HitboxOffX = 0.0f;
    float        HitboxOffY = 0.0f;
    Entity*      PrevEntity = NULL;
    Entity*      NextEntity = NULL;
    ObjectList*  List = NULL;
    Entity*      PrevEntityInList = NULL;
    Entity*      NextEntityInList = NULL;

    void ApplyMotion();
    void Animate();
    void SetAnimation(int animation, int frame);
    void ResetAnimation(int animation, int frame);
    virtual void Create();
    virtual void Update();
    virtual void OnAnimationFinish();
    virtual void RenderEarly();
    virtual void Render(int CamX, int CamY);
    virtual void RenderLate();
    virtual void Dispose();
};

#endif /* ENTITY_H */
