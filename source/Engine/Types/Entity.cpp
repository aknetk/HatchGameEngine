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

    float        XSpeed = 0.0f;
    float        YSpeed = 0.0f;
    float        GroundSpeed = 0.0f;
    float        Gravity = 0.0f;
    int          Ground = false;

    int          OnScreen = true;
    float        OnScreenHitboxW = 0.0f;
    float        OnScreenHitboxH = 0.0f;

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
    float        CurrentFrameTimeLeft = 0.0;
    float        AnimationSpeedMult = 1.0;
    int          AutoAnimate = true;

    float        HitboxW = 0.0f;
    float        HitboxH = 0.0f;
    float        HitboxOffX = 0.0f;
    float        HitboxOffY = 0.0f;

    int          Persistent = false;

    Entity*      PrevEntity = NULL;
    Entity*      NextEntity = NULL;

    ObjectList*  List = NULL;
    Entity*      PrevEntityInList = NULL;
    Entity*      NextEntityInList = NULL;
};
#endif

#include <Engine/Types/Entity.h>

PUBLIC void Entity::ApplyMotion() {
    YSpeed += Gravity;
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

PUBLIC bool Entity::CollideWithObject(Entity* other) {
    float sourceX = std::floor(this->X + this->HitboxOffX);
    float sourceY = std::floor(this->Y + this->HitboxOffY);
    float otherX = std::floor(other->X + other->HitboxOffX);
    float otherY = std::floor(other->Y + other->HitboxOffY);

    float otherHitboxW = (other->HitboxW) * 0.5;
    float otherHitboxH = (other->HitboxH) * 0.5;
    float sourceHitboxW = (this->HitboxW) * 0.5;
    float sourceHitboxH = (this->HitboxH) * 0.5;

    if (otherY + otherHitboxH < sourceY - sourceHitboxH ||
        otherY - otherHitboxH > sourceY + sourceHitboxH ||
        sourceX - sourceHitboxW > otherX + otherHitboxW ||
        sourceX + sourceHitboxW < otherX - otherHitboxW)
        return false;

    return true;
}
PUBLIC int  Entity::SolidCollideWithObject(Entity* other, int flag) {
    // NOTE: "flag" might be "UseGroundSpeed"
    float initialOtherX = (other->X);
    float initialOtherY = (other->Y);
    float sourceX = std::floor(this->X);
    float sourceY = std::floor(this->Y);
    float otherX = std::floor(initialOtherX);
    float otherY = std::floor(initialOtherY);
    float otherNewX = initialOtherX;
    int collideSideHori = 0;
    int collideSideVert = 0;

    float otherHitboxW = (other->HitboxW) * 0.5;
    float otherHitboxH = (other->HitboxH) * 0.5;
    float otherHitboxWSq = (other->HitboxW - 2.0) * 0.5;
    float otherHitboxHSq = (other->HitboxH - 2.0) * 0.5;
    float sourceHitboxW = (this->HitboxW) * 0.5;
    float sourceHitboxH = (this->HitboxH) * 0.5;

    // NOTE: Keep this.
    // if ( other_X <= (sourceHitbox->Right + sourceHitbox->Left + 2 * source_X) >> 1 )
    // if ( other_X <= (sourceHitbox->Right + sourceHitbox->Left) / 2 + source_X )
    // if ( other_X <= source_X + (sourceHitbox->Right + sourceHitbox->Left) / 2 )
    // if other->X <= this->X + this->HitboxCenterX

    // Check squeezed vertically
    if (sourceY + (-sourceHitboxH + this->HitboxOffY) < initialOtherY + otherHitboxHSq &&
        sourceY + (sourceHitboxH + this->HitboxOffY) > initialOtherY - otherHitboxHSq) {
        // if other->X <= this->X + this->HitboxCenterX
        if (otherX <= sourceX) {
            if (otherX + otherHitboxW >= sourceX - sourceHitboxW) {
                collideSideHori = 2;
                otherNewX = this->X + ((-sourceHitboxW + this->HitboxOffX) - (otherHitboxW + other->HitboxOffX));
            }
        }
        else {
            if (otherX - otherHitboxW < sourceX + sourceHitboxW) {
                collideSideHori = 3;
                otherNewX = this->X + ((sourceHitboxW + this->HitboxOffX) - (-otherHitboxW + other->HitboxOffX));
            }
        }
    }

    // Check squeezed horizontally
    if (sourceX - sourceHitboxW < initialOtherX + otherHitboxWSq &&
        sourceX + sourceHitboxW > initialOtherX - otherHitboxWSq) {
        // if other->Y <= this->Y + this->HitboxCenterY
        if (otherY <= sourceY) {
            if (otherY + (otherHitboxH + other->HitboxOffY) >= sourceY + (-sourceHitboxH + this->HitboxOffY)) {
                collideSideVert = 1;
                otherY = this->Y + ((-sourceHitboxH + this->HitboxOffY) - (otherHitboxH + other->HitboxOffY));
            }
        }
        else {
            if (otherY + (-otherHitboxH + other->HitboxOffY) < sourceY + (sourceHitboxH + this->HitboxOffY)) {
                collideSideVert = 4;
                otherY = this->Y + ((sourceHitboxH + this->HitboxOffY) - (-otherHitboxH + other->HitboxOffY));
            }
        }
    }

    float deltaSquaredX1 = otherNewX - (other->X);
    float deltaSquaredX2 = initialOtherX - (other->X);
    float deltaSquaredY1 = otherY - (other->Y);
    float deltaSquaredY2 = initialOtherY - (other->Y);

    if (collideSideHori && collideSideVert) {
        // print
        //     " deltaSquaredX1: " + deltaSquaredX1 +
        //     " deltaSquaredX2: " + deltaSquaredX2 +
        //     " deltaSquaredY1: " + deltaSquaredY1 +
        //     " deltaSquaredY2: " + deltaSquaredY2;
        // print
        //     " collideSideHori: " + collideSideHori +
        //     " collideSideVert: " + collideSideVert;
    }

    deltaSquaredX1 *= deltaSquaredX1;
    deltaSquaredX2 *= deltaSquaredX2;
    deltaSquaredY1 *= deltaSquaredY1;
    deltaSquaredY2 *= deltaSquaredY2;

    int v47 = collideSideVert, v48;
    int v46 = collideSideHori;

    if (collideSideHori != 0 || collideSideVert != 0) {
        if (deltaSquaredX1 + deltaSquaredY2 >= deltaSquaredX2 + deltaSquaredY1) {
            if (collideSideVert || !collideSideHori) {
                other->X = initialOtherX;
                other->Y = otherY;
                if (flag == 1) {
                    if (collideSideVert != 1) {
                        if (collideSideVert == 4 && other->YSpeed < 0.0) {
                            other->YSpeed = 0.0;
                            return v47;
                        }
                        return v47;
                    }

                    if (other->YSpeed > 0.0)
                        other->YSpeed = 0.0;
                    if (!other->Ground && other->YSpeed >= 0.0) {
                        other->GroundSpeed = other->XSpeed;
                        other->Angle = 0;
                        other->Ground = true;
                    }
                }
                return v47;
            }
        }
        else {
            if (collideSideVert && !collideSideHori) {
                other->X = initialOtherX;
                other->Y = otherY;
                if (flag == 1) {
                    if (collideSideVert != 1) {
                        if (collideSideVert == 4 && other->YSpeed < 0.0) {
                            other->YSpeed = 0.0;
                            return v47;
                        }
                        return v47;
                    }

                    if (other->YSpeed > 0.0)
                        other->YSpeed = 0.0;
                    if (!other->Ground && other->YSpeed >= 0.0) {
                        other->GroundSpeed = other->XSpeed;
                        other->Angle = 0;
                        other->Ground = true;
                    }
                }
                return v47;
            }
        }

        other->X = otherNewX;
        other->Y = initialOtherY;
        if (flag == 1) {
            float v50;
            if (other->Ground) {
                v50 = other->GroundSpeed;
                if (other->AngleMode == 2)
                    v50 = -v50;
            }
            else {
                v50 = other->XSpeed;
            }

            if (v46 == 2) {
                if (v50 <= 0.0)
                    return v46;
            }
            else if (v46 != 3 || v50 >= 0.0) {
                return v46;
            }

            other->GroundSpeed = 0.0;
            other->XSpeed = 0.0;
        }
        return v46;
    }
    return 0;
}
PUBLIC bool Entity::TopSolidCollideWithObject(Entity* other, int flag) {
    // NOTE: "flag" might be "UseGroundSpeed"
    float initialOtherX = (other->X + other->HitboxOffX);
    float initialOtherY = (other->Y + other->HitboxOffY);
    float sourceX = std::floor(this->X + this->HitboxOffX);
    float sourceY = std::floor(this->Y + this->HitboxOffY);
    float otherX = std::floor(initialOtherX);
    float otherY = std::floor(initialOtherY);
    float otherYMinusYSpeed = std::floor(initialOtherY - other->YSpeed);

    float otherHitboxW = (other->HitboxW) * 0.5;
    float otherHitboxH = (other->HitboxH) * 0.5;
    float sourceHitboxW = (this->HitboxW) * 0.5;
    float sourceHitboxH = (this->HitboxH) * 0.5;

    if (otherY + otherHitboxH < sourceY - sourceHitboxH ||
        otherYMinusYSpeed + otherHitboxH > sourceY + sourceHitboxH ||
        sourceX - sourceHitboxW >= otherX + otherHitboxW ||
        sourceX + sourceHitboxW <= otherX - otherHitboxW ||
        other->YSpeed < 0.0)
        return false;

    other->Y = this->Y + ((-sourceHitboxH + this->HitboxOffY) - (otherHitboxH + other->HitboxOffY));
    if (flag) {
        other->YSpeed = 0.0;
        if (!other->Ground) {
            other->GroundSpeed = other->XSpeed;
            other->Angle = 0;
            other->Ground = true;
        }
    }
    return true;
}

PUBLIC VIRTUAL void Entity::Create() {

}

PUBLIC VIRTUAL void Entity::Update() {
}
PUBLIC VIRTUAL void Entity::UpdateLate() {
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
