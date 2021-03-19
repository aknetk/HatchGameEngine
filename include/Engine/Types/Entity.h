#ifndef ENGINE_TYPES_ENTITY_H
#define ENGINE_TYPES_ENTITY_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED

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
    float        XSpeed = 0.0f;
    float        YSpeed = 0.0f;
    float        GroundSpeed = 0.0f;
    float        Gravity = 0.0f;
    int          Ground = false;
    int          OnScreen = true;
    float        OnScreenHitboxW = 0.0f;
    float        OnScreenHitboxH = 0.0f;
    int          ViewRenderFlag = 1;
    float        RenderRegionW = 0.0f;
    float        RenderRegionH = 0.0f;
    int          Angle = 0;
    int          AngleMode = 0;
    float        Rotation = 0.0;
    int          AutoPhysics = false;
    int          Priority = 0;
    int          PriorityListIndex = -1;
    int          PriorityOld = 0;
    int          Sprite = -1;
    int          CurrentAnimation = -1;
    int          CurrentFrame = -1;
    int          CurrentFrameCount = 0;
    float        CurrentFrameTimeLeft = 0.0;
    float        AnimationSpeedMult = 1.0;
    int          AnimationSpeedAdd = 0;
    int          AutoAnimate = true;
    float        HitboxW = 0.0f;
    float        HitboxH = 0.0f;
    float        HitboxOffX = 0.0f;
    float        HitboxOffY = 0.0f;
    int          FlipFlag = 0;
    int          Persistent = false;
    int          Interactable = true;
    Entity*      PrevEntity = NULL;
    Entity*      NextEntity = NULL;
    ObjectList*  List = NULL;
    Entity*      PrevEntityInList = NULL;
    Entity*      NextEntityInList = NULL;

    void ApplyMotion();
    void Animate();
    void SetAnimation(int animation, int frame);
    void ResetAnimation(int animation, int frame);
    bool CollideWithObject(Entity* other);
    int  SolidCollideWithObject(Entity* other, int flag);
    bool TopSolidCollideWithObject(Entity* other, int flag);
    void ApplyPhysics();
    virtual void GameStart();
    virtual void Create(int flag);
    virtual void Setup();
    virtual void UpdateEarly();
    virtual void Update();
    virtual void UpdateLate();
    virtual void OnAnimationFinish();
    virtual void RenderEarly();
    virtual void Render(int CamX, int CamY);
    virtual void RenderLate();
    virtual void Dispose();
};

#endif /* ENGINE_TYPES_ENTITY_H */
