#if INTERFACE
#include <Engine/Types/Entity.h>
#include <Engine/Bytecode/BytecodeObjectManager.h>

class BytecodeObject : public Entity {
public:
    ObjInstance* Instance = NULL;
    HashMap<VMValue>* Properties;
};
#endif

#include <Engine/Bytecode/BytecodeObject.h>
#include <Engine/Bytecode/BytecodeObjectManager.h>
#include <Engine/Bytecode/Compiler.h>
#include <Engine/Bytecode/StandardLibrary.h>
#include <Engine/Scene.h>

#define LINK_INT(VAR) Instance->Fields->Put(#VAR, INTEGER_LINK_VAL(&VAR))
#define LINK_DEC(VAR) Instance->Fields->Put(#VAR, DECIMAL_LINK_VAL(&VAR))
#define LINK_BOOL(VAR) Instance->Fields->Put(#VAR, INTEGER_LINK_VAL(&VAR))

PUBLIC void BytecodeObject::Link(ObjInstance* instance) {
    Instance = instance;
    Instance->EntityPtr = this;
    Properties = new HashMap<VMValue>(NULL, 4);

    LINK_DEC(X);
    LINK_DEC(Y);
    LINK_DEC(Z);
    LINK_DEC(XSpeed);
    LINK_DEC(YSpeed);
    LINK_DEC(GroundSpeed);
    LINK_DEC(Gravity);
    LINK_INT(AutoPhysics);
    LINK_INT(Angle);
    LINK_INT(AngleMode);
    LINK_INT(Ground);
    LINK_DEC(Rotation);
    // LINK_DEC(Timer);
    LINK_INT(Priority);
    // LINK_INT(PriorityListIndex);
    LINK_INT(Sprite);
    LINK_INT(CurrentAnimation);
    LINK_INT(CurrentFrame);
    LINK_DEC(CurrentFrameTimeLeft);
    LINK_DEC(AnimationSpeedMult);
    LINK_INT(AutoAnimate);

    LINK_INT(OnScreen);
    LINK_DEC(OnScreenHitboxW);
    LINK_DEC(OnScreenHitboxH);

    LINK_DEC(HitboxW);
    LINK_DEC(HitboxH);
    LINK_DEC(HitboxOffX);
    LINK_DEC(HitboxOffY);

    LINK_BOOL(Active);
    LINK_BOOL(Pauseable);
}

#undef LINK

PUBLIC bool BytecodeObject::RunFunction(const char* f) {
    // NOTE:
    // If the function doesn't exist, this is not an error VM side,
    // treat whatever we call from C++ as a virtual-like function.
    Uint32 hash = FNV1A::EncryptString(f);
    if (!Instance->Class->Methods->Exists(hash))
        return true;

    VMThread* thread = BytecodeObjectManager::Threads + 0;

    VMValue* StackTop = thread->StackTop;

    thread->Push(OBJECT_VAL(Instance));
    thread->RunInvoke(hash, 0 /* arity */);

    int stackChange = thread->StackTop - StackTop;
    if (stackChange) {
        // printf("BytecodeObject::RunFunction(%s) Stack Change: %d\n", f, stackChange);
        // BytecodeObjectManager::PrintStack();
    }
    thread->StackTop = StackTop;

    // NOTE: The ObjInstance* value is left on the stack after this.
    // BytecodeObjectManager::ResetStack();
    // BytecodeObjectManager::PrintStack();

    return false;
}

// Events called from C++
PUBLIC void BytecodeObject::Create() {
    if (!Instance) return;

    // Set defaults
    Active = true;
    Priority = 0;
    PriorityOld = 0;
    PriorityListIndex = -1;
    Sprite = -1;
    CurrentAnimation = -1;
    CurrentFrame = -1;
    CurrentFrameTimeLeft = 0;
    AnimationSpeedMult = 1.0f;
    // X = 0.0f;
    // Y = 0.0f;
    XSpeed = 0;
    YSpeed = 0;
    Angle = 0;
    // Timer = 0;
    Pauseable = true;

    RunFunction("Create");
    if (Sprite >= 0 && CurrentAnimation < 0) {
        SetAnimation(0, 0);
    }
}
PUBLIC void BytecodeObject::Update() {
    if (!Active) return;
    if (!Instance) return;

    RunFunction("Update");
}
PUBLIC void BytecodeObject::UpdateLate() {
    if (!Active) return;
    if (!Instance) return;

    RunFunction("UpdateLate");

    if (AutoAnimate)
        Animate();
    if (AutoPhysics)
        ApplyMotion();
}
PUBLIC void BytecodeObject::RenderEarly() {
    if (!Active) return;
    if (!Instance) return;

    RunFunction("RenderEarly");
}
PUBLIC void BytecodeObject::Render(int CamX, int CamY) {
    if (!Active) return;
    if (!Instance) return;

    if (RunFunction("Render")) {
        // Default render
    }
}
PUBLIC void BytecodeObject::RenderLate() {
    if (!Active) return;
    if (!Instance) return;

    RunFunction("RenderLate");
}
PUBLIC void BytecodeObject::OnAnimationFinish() {
    RunFunction("OnAnimationFinish");
}

PUBLIC void BytecodeObject::Dispose() {
    Entity::Dispose();
    Properties->Dispose();
    if (Instance) {
        // Instance->Fields->ForAll(BytecodeObjectManager::FreeValue);
        // delete Instance->Fields;
        // Memory::Free(Instance);
        Instance = NULL;
    }
}

// Events/methods called from VM
bool _TestCollision(BytecodeObject* other, BytecodeObject* self) {
    if (!other->Active) return false;
    // if (!other->Instance) return false;
    if (other->HitboxW == 0.0f ||
        other->HitboxH == 0.0f) return false;

    if (other->X + other->HitboxW / 2.0f >= self->X - self->HitboxW / 2.0f &&
        other->Y + other->HitboxH / 2.0f >= self->Y - self->HitboxH / 2.0f &&
        other->X - other->HitboxW / 2.0f  < self->X + self->HitboxW / 2.0f &&
        other->Y - other->HitboxH / 2.0f  < self->Y + self->HitboxH / 2.0f) {
        BytecodeObjectManager::Globals->Put("other", OBJECT_VAL(other->Instance));
        return true;
    }
    return false;
}
PUBLIC STATIC VMValue BytecodeObject::VM_SetAnimation(int argCount, VMValue* args, Uint32 threadID) {
    Entity* self = (Entity*)AS_INSTANCE(args[0])->EntityPtr;
    int animation = AS_INTEGER(args[1]);
    int frame = AS_INTEGER(args[2]);

    ISprite* sprite = Scene::SpriteList[self->Sprite]->AsSprite;
    if (!(animation > -1 && (size_t)animation < sprite->Animations.size())) {
        BytecodeObjectManager::Threads[threadID].ThrowError(false, "Animation %d is not in bounds of sprite.", animation);
        return NULL_VAL;
    }
    if (!(frame > -1 && (size_t)frame < sprite->Animations[animation].Frames.size())) {
        BytecodeObjectManager::Threads[threadID].ThrowError(false, "Frame %d is not in bounds of animation %d.", frame, animation);
        return NULL_VAL;
    }

    self->SetAnimation(animation, frame);
    return NULL_VAL;
}
PUBLIC STATIC VMValue BytecodeObject::VM_ResetAnimation(int argCount, VMValue* args, Uint32 threadID) {
    Entity* self = (Entity*)AS_INSTANCE(args[0])->EntityPtr;
    int animation = AS_INTEGER(args[1]);
    int frame = AS_INTEGER(args[2]);

    ISprite* sprite = Scene::SpriteList[self->Sprite]->AsSprite;
    if (!(animation < sprite->Animations.size())) {
        BytecodeObjectManager::Threads[threadID].ThrowError(false, "Animation %d is not in bounds of sprite.", animation);
        return NULL_VAL;
    }
    if (!(frame < sprite->Animations[animation].Frames.size())) {
        BytecodeObjectManager::Threads[threadID].ThrowError(false, "Frame %d is not in bounds of animation %d.", frame, animation);
        return NULL_VAL;
    }

    self->ResetAnimation(animation, frame);
    return NULL_VAL;
}
PUBLIC STATIC VMValue BytecodeObject::VM_Animate(int argCount, VMValue* args, Uint32 threadID) {
    Entity* self = (Entity*)AS_INSTANCE(args[0])->EntityPtr;
    self->Animate();
    return NULL_VAL;
}
PUBLIC STATIC VMValue BytecodeObject::VM_AddToRegistry(int argCount, VMValue* args, Uint32 threadID) {
    Entity* self     = (Entity*)AS_INSTANCE(args[0])->EntityPtr;
    char*   registry = AS_CSTRING(args[1]);

    ObjectList* objectList;
    if (!Scene::ObjectRegistries->Exists(registry)) {
        objectList = new ObjectList();
        objectList->Registry = true;
        Scene::ObjectRegistries->Put(registry, objectList);
    }
    else {
        objectList = Scene::ObjectRegistries->Get(registry);
    }

    objectList->Add(self);

    return NULL_VAL;
}
PUBLIC STATIC VMValue BytecodeObject::VM_RemoveFromRegistry(int argCount, VMValue* args, Uint32 threadID) {
    Entity* self     = (Entity*)AS_INSTANCE(args[0])->EntityPtr;
    char*   registry = AS_CSTRING(args[1]);

    ObjectList* objectList;
    if (!Scene::ObjectRegistries->Exists(registry)) {
        return NULL_VAL;
    }
    objectList = Scene::ObjectRegistries->Get(registry);

    objectList->Remove(self);

    return NULL_VAL;
}
PUBLIC STATIC VMValue BytecodeObject::VM_ApplyMotion(int argCount, VMValue* args, Uint32 threadID) {
    Entity* self = (Entity*)AS_INSTANCE(args[0])->EntityPtr;
    self->ApplyMotion();
    return NULL_VAL;
}
PUBLIC STATIC VMValue BytecodeObject::VM_InView(int argCount, VMValue* args, Uint32 threadID) {
    // Entity* self = (Entity*)AS_INSTANCE(args[0])->EntityPtr;
    int   view   = StandardLibrary::GetInteger(args, 1);
    float x      = StandardLibrary::GetDecimal(args, 2);
    float y      = StandardLibrary::GetDecimal(args, 3);
    float w      = StandardLibrary::GetDecimal(args, 4);
    float h      = StandardLibrary::GetDecimal(args, 5);

    if (x + w >= Scene::Views[view].X &&
        y + h >= Scene::Views[view].Y &&
        x      < Scene::Views[view].X + Scene::Views[view].Width &&
        y      < Scene::Views[view].Y + Scene::Views[view].Height)
        return INTEGER_VAL(true);

    return INTEGER_VAL(false);
}
PUBLIC STATIC VMValue BytecodeObject::VM_CollidedWithObject(int argCount, VMValue* args, Uint32 threadID) {
    StandardLibrary::CheckArgCount(argCount, 2);

    if (IS_INSTANCE(args[1])) {
        BytecodeObject* self = (BytecodeObject*)AS_INSTANCE(args[0])->EntityPtr;
        BytecodeObject* other = (BytecodeObject*)AS_INSTANCE(args[1])->EntityPtr;
        return INTEGER_VAL(_TestCollision(other, self));
    }

    if (!Scene::ObjectLists) return INTEGER_VAL(false);
    if (!Scene::ObjectRegistries) return INTEGER_VAL(false);

    char* object = StandardLibrary::GetString(args, 1);
    if (!Scene::ObjectRegistries->Exists(object)) {
        if (!Scene::ObjectLists->Exists(object))
            return INTEGER_VAL(false);
    }

    BytecodeObject* self = (BytecodeObject*)AS_INSTANCE(args[0])->EntityPtr;
    if (self->HitboxW == 0.0f ||
        self->HitboxH == 0.0f) return INTEGER_VAL(false);

    BytecodeObject* other = NULL;
    ObjectList* objectList = Scene::ObjectLists->Get(object);

    other = (BytecodeObject*)objectList->EntityFirst;
    for (Entity* next; other; other = (BytecodeObject*)next) {
        next = other->NextEntityInList;
        if (_TestCollision(other, self))
            return INTEGER_VAL(true);
    }

    return INTEGER_VAL(false);
}
PUBLIC STATIC VMValue BytecodeObject::VM_GetHitboxFromSprite(int argCount, VMValue* args, Uint32 threadID) {
    StandardLibrary::CheckArgCount(argCount, 5);
    Entity* self    = (Entity*)AS_INSTANCE(args[0])->EntityPtr;
    ISprite* sprite = StandardLibrary::GetSprite(args, 1);
    int animation   = StandardLibrary::GetInteger(args, 2);
    int frame       = StandardLibrary::GetInteger(args, 3);
    int hitbox      = StandardLibrary::GetInteger(args, 4);

    if (!(animation > -1 && (size_t)animation < sprite->Animations.size())) {
        BytecodeObjectManager::Threads[threadID].ThrowError(false, "Animation %d is not in bounds of sprite.", animation);
        return NULL_VAL;
    }
    if (!(frame > -1 && (size_t)frame < sprite->Animations[animation].Frames.size())) {
        BytecodeObjectManager::Threads[threadID].ThrowError(false, "Frame %d is not in bounds of animation %d.", frame, animation);
        return NULL_VAL;
    }

    AnimFrame frameO = sprite->Animations[animation].Frames[frame];

    if (!(hitbox > -1 && hitbox < frameO.BoxCount)) {
        BytecodeObjectManager::Threads[threadID].ThrowError(false, "Hitbox %d is not in bounds of frame %d.", hitbox, frame);
        return NULL_VAL;
    }

    CollisionBox box = frameO.Boxes[hitbox];
    self->HitboxW = box.Right - box.Left;
    self->HitboxH = box.Bottom - box.Top;
    self->HitboxOffX = box.Left + self->HitboxW * 0.5f;
    self->HitboxOffY = box.Top + self->HitboxH * 0.5f;

    return NULL_VAL;
}

/*
bool Entity::CollideWithObject
int  Entity::SolidCollideWithObject
bool Entity::TopSolidCollideWithObject
*/
PUBLIC STATIC VMValue BytecodeObject::VM_CollideWithObject(int argCount, VMValue* args, Uint32 threadID) {
    StandardLibrary::CheckArgCount(argCount, 2);
    BytecodeObject* self = (BytecodeObject*)AS_INSTANCE(args[0])->EntityPtr;
    BytecodeObject* other = (BytecodeObject*)AS_INSTANCE(args[1])->EntityPtr;
    return INTEGER_VAL(self->CollideWithObject(other));
}
PUBLIC STATIC VMValue BytecodeObject::VM_SolidCollideWithObject(int argCount, VMValue* args, Uint32 threadID) {
    StandardLibrary::CheckArgCount(argCount, 3);
    BytecodeObject* self = (BytecodeObject*)AS_INSTANCE(args[0])->EntityPtr;
    BytecodeObject* other = (BytecodeObject*)AS_INSTANCE(args[1])->EntityPtr;
    int flag = StandardLibrary::GetInteger(args, 2);
    return INTEGER_VAL(self->SolidCollideWithObject(other, flag));
}
PUBLIC STATIC VMValue BytecodeObject::VM_TopSolidCollideWithObject(int argCount, VMValue* args, Uint32 threadID) {
    StandardLibrary::CheckArgCount(argCount, 3);
    BytecodeObject* self = (BytecodeObject*)AS_INSTANCE(args[0])->EntityPtr;
    BytecodeObject* other = (BytecodeObject*)AS_INSTANCE(args[1])->EntityPtr;
    int flag = StandardLibrary::GetInteger(args, 2);
    return INTEGER_VAL(self->TopSolidCollideWithObject(other, flag));
}

PUBLIC STATIC VMValue BytecodeObject::VM_PropertyExists(int argCount, VMValue* args, Uint32 threadID) {
    BytecodeObject* self = (BytecodeObject*)AS_INSTANCE(args[0])->EntityPtr;
    char* property = AS_CSTRING(args[1]);
    if (self->Properties->Exists(property))
        return INTEGER_VAL(1);
    return INTEGER_VAL(0);
}
PUBLIC STATIC VMValue BytecodeObject::VM_PropertyGet(int argCount, VMValue* args, Uint32 threadID) {
    BytecodeObject* self = (BytecodeObject*)AS_INSTANCE(args[0])->EntityPtr;
    char* property = AS_CSTRING(args[1]);
    if (!self->Properties->Exists(property))
        return NULL_VAL;
    return self->Properties->Get(property);
}
