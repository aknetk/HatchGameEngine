#if INTERFACE
#include <Engine/Includes/Standard.h>
#include <Engine/Application.h>
#include <Engine/Graphics.h>
#include <Engine/ResourceTypes/ISprite.h>
#include <Engine/Scene.h>

#include <Engine/Types/EntityTypes.h>
#include <Engine/Includes/HashMap.h>

need_t ObjectList;

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
};
#endif

#include <Engine/Types/Entity.h>

PUBLIC void Entity::ApplyMotion() {
    X += XSpeed;
    Y += YSpeed;
}
PUBLIC void Entity::Animate() {
    if (Sprite < 0) return;
    if (Sprite >= Scene::SpriteList.size()) return;

    ISprite* sprite = Scene::SpriteList[Sprite]->AsSprite;

    if (CurrentAnimation < 0) return;
    if (CurrentAnimation >= sprite->Animations.size()) return;

    if (CurrentFrameTimeLeft > 0.0f) {
        CurrentFrameTimeLeft -= (sprite->Animations[CurrentAnimation].AnimationSpeed * AnimationSpeedMult);
        if (CurrentFrameTimeLeft <= 0.0f) {
            CurrentFrame += 1.0f;
            if (CurrentFrame >= (int)sprite->Animations[CurrentAnimation].Frames.size()) {
                CurrentFrame  = sprite->Animations[CurrentAnimation].FrameToLoop;
                OnAnimationFinish();
            }

            sprite = Scene::SpriteList[Sprite]->AsSprite;
            CurrentFrameTimeLeft = sprite->Animations[CurrentAnimation].Frames[CurrentFrame].Duration;
        }
    }
    else {
        CurrentFrameTimeLeft = sprite->Animations[CurrentAnimation].Frames[CurrentFrame].Duration;
    }
}
PUBLIC void Entity::SetAnimation(int animation, int frame) {
    if (CurrentAnimation != animation)
        ResetAnimation(animation, frame);
}
PUBLIC void Entity::ResetAnimation(int animation, int frame) {
    if (Sprite < 0) return;

    ISprite* sprite = Scene::SpriteList[Sprite]->AsSprite;

    assert(animation < sprite->Animations.size());
    assert(frame < sprite->Animations[animation].Frames.size());

    CurrentAnimation = animation;
    CurrentFrame = frame;
    CurrentFrameTimeLeft = sprite->Animations[CurrentAnimation].Frames[CurrentFrame].Duration;
}


PUBLIC VIRTUAL void Entity::Create() {

}

PUBLIC VIRTUAL void Entity::Update() {

}

PUBLIC VIRTUAL void Entity::OnAnimationFinish() {

}

PUBLIC VIRTUAL void Entity::RenderEarly() {

}

PUBLIC VIRTUAL void Entity::Render(int CamX, int CamY) {

}

PUBLIC VIRTUAL void Entity::RenderLate() {

}

PUBLIC VIRTUAL void Entity::Dispose() {
}
