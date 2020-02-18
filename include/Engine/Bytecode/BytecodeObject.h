#ifndef BYTECODEOBJECT_H
#define BYTECODEOBJECT_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Types/Entity.h>
#include <Engine/Bytecode/BytecodeObjectManager.h>

class BytecodeObject : public Entity {
public:
    ObjInstance* Instance = NULL;
    HashMap<VMValue>* Properties;

    void Link(ObjInstance* instance);
    bool RunFunction(const char* f);
    void Create();
    void Update();
    void RenderEarly();
    void Render(int CamX, int CamY);
    void RenderLate();
    void OnAnimationFinish();
    void Dispose();
    static VMValue VM_SetAnimation(int argCount, VMValue* args, Uint32 threadID);
    static VMValue VM_ResetAnimation(int argCount, VMValue* args, Uint32 threadID);
    static VMValue VM_Animate(int argCount, VMValue* args, Uint32 threadID);
    static VMValue VM_AddToRegistry(int argCount, VMValue* args, Uint32 threadID);
    static VMValue VM_RemoveFromRegistry(int argCount, VMValue* args, Uint32 threadID);
    static VMValue VM_ApplyMotion(int argCount, VMValue* args, Uint32 threadID);
    static VMValue VM_InView(int argCount, VMValue* args, Uint32 threadID);
    static VMValue VM_CollidedWithObject(int argCount, VMValue* args, Uint32 threadID);
    static VMValue VM_PropertyExists(int argCount, VMValue* args, Uint32 threadID);
    static VMValue VM_PropertyGet(int argCount, VMValue* args, Uint32 threadID);
};

#endif /* BYTECODEOBJECT_H */
