#if INTERFACE

#include <Engine/Includes/Standard.h>

#include <Engine/Types/Entity.h>

class ObjectList {
public:
    int     EntityCount = 0;
    Entity* EntityFirst = NULL;
    Entity* EntityLast = NULL;

    bool            Registry = false;
    vector<Entity*> List;

    char ObjectName[256];
    double AverageTime = 0.0;
    double AverageItemCount = 0;

    Entity* (*SpawnFunction)() = NULL;
};
#endif

#include <Engine/Types/ObjectList.h>

#include <Engine/Application.h>
#include <Engine/Diagnostics/Memory.h>

// Double linked-list functions
PUBLIC void    ObjectList::Add(Entity* obj) {
    if (Registry) {
        List.push_back(obj);
        EntityCount++;
        return;
    }

    // Set "prev" of obj to last
    obj->PrevEntityInList = EntityLast;
    obj->NextEntityInList = NULL;

    // If the last exists, set that ones "next" to obj
    if (EntityLast)
        EntityLast->NextEntityInList = obj;

    // Set obj as the first if there is not one
    if (!EntityFirst)
        EntityFirst = obj;

    EntityLast = obj;

    EntityCount++;
}
PUBLIC void    ObjectList::Remove(Entity* obj) {
    if (obj == NULL) return;

    if (Registry) {
        for (int i = 0, iSz = (int)List.size(); i < iSz; i++) {
            if (List[i] == obj) {
                List.erase(List.begin() + i);
                break;
            }
        }
        EntityCount--;
        return;
    }

    if (EntityFirst == obj)
        EntityFirst = obj->NextEntityInList;

    if (EntityLast == obj)
        EntityLast = obj->PrevEntityInList;

    if (obj->PrevEntityInList)
        obj->PrevEntityInList->NextEntityInList = obj->NextEntityInList;

    if (obj->NextEntityInList)
        obj->NextEntityInList->PrevEntityInList = obj->PrevEntityInList;

    EntityCount--;
}
PUBLIC void    ObjectList::Clear() {
    if (Registry) {
        List.clear();
        EntityCount = 0;
        return;
    }

    EntityCount = 0;
    EntityFirst = NULL;
    EntityLast = NULL;

    AverageTime = 0.0;
    AverageItemCount = 0;
}

// ObjectList functions
PUBLIC void    ObjectList::ReserveStatic(int count) {
    // StaticEntities.reserve(count);
}
PUBLIC Entity* ObjectList::AddStatic(Entity* obj) {
    // StaticEntities.push_back(obj);
    Add(obj);
    return obj;
}
PUBLIC Entity* ObjectList::AddDynamic(Entity* obj) {
    // DynamicEntities.push_back(obj);
    return AddStatic(obj);
}
PUBLIC bool    ObjectList::RemoveDynamic(Entity* obj) {
    Remove(obj);
    // NOTE: The scene does it's own removal before this function,
    //       and then calls ObjectList::RemoveDynamic.
    return true;
}

PUBLIC Entity* ObjectList::GetNth(int n) {
    if (Registry)
        return List[n];

    Entity* ent = EntityFirst;
    for (ent = EntityFirst; ent != NULL && n > 0; ent = ent->NextEntityInList, n--);
    return ent;
}
PUBLIC Entity* ObjectList::GetClosest(int x, int y) {
    if (EntityCount == 1)
        return EntityFirst;

    Entity* closest = NULL;

    int xD, yD;
    int smallestDistance = 0x7FFFFFFF;

    for (Entity* ent = EntityFirst; ent != NULL; ent = ent->NextEntityInList) {
        xD = ent->X - x; xD *= xD;
        yD = ent->Y - y; yD *= yD;
        if (smallestDistance > xD + yD) {
            smallestDistance = xD + yD;
            closest = ent;
        }
    }

    return closest;
}

PUBLIC void    ObjectList::Dispose() {
    // for (Uint32 i = 0; i < StaticEntities.size(); i++) {
    //     // obj->Dispose();
    //     Memory::Free(StaticEntities[i]);
    // }
    // for (Uint32 i = 0; i < DynamicEntities.size(); i++) {
    //     // obj->Dispose();
    //     Memory::Free(DynamicEntities[i]);
    // }
}
PUBLIC         ObjectList::~ObjectList() {
    Dispose();
}

PUBLIC int     ObjectList::Count() {
    return EntityCount;
}
