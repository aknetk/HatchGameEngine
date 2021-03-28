#if INTERFACE

#include <Engine/Includes/Standard.h>

#include <Engine/Types/Entity.h>

class DrawGroupList {
public:
    int      EntityCount = 0;
    int      EntityCapacity = 0x1000;
    Entity** Entities = NULL;
};
#endif

#include <Engine/Types/DrawGroupList.h>

#include <Engine/Application.h>
#include <Engine/Diagnostics/Memory.h>

PUBLIC         DrawGroupList::DrawGroupList() {
    Init();
}

// Double linked-list functions
PUBLIC int    DrawGroupList::Add(Entity* obj) {
    for (int i = 0, iSz = EntityCapacity; i < iSz; i++) {
        if (Entities[i] == NULL) {
            Entities[i] = obj;
            EntityCount++;
            return i;
        }
    }
    return -1;
}
PUBLIC void    DrawGroupList::Remove(Entity* obj) {
    for (int i = 0, iSz = EntityCapacity; i < iSz; i++) {
        if (Entities[i] == obj) {
            Entities[i] = NULL;
            EntityCount--;
            return;
        }
    }
}
PUBLIC void    DrawGroupList::Clear() {
    for (int i = 0, iSz = EntityCapacity; i < iSz; i++) {
        Entities[i] = NULL;
    }
    EntityCount = 0;
}

PUBLIC void    DrawGroupList::Init() {
    EntityCapacity = 0x1000;
    Entities = (Entity**)Memory::TrackedCalloc("DrawGroupList", sizeof(Entity*), EntityCapacity);
    EntityCount = 0;
}
PUBLIC void    DrawGroupList::Dispose() {
    printf("Dispose();\n");
    Memory::Free(Entities);
}
PUBLIC         DrawGroupList::~DrawGroupList() {
    // Dispose();
}

PUBLIC int     DrawGroupList::Count() {
    return EntityCount;
}
