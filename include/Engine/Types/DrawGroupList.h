#ifndef ENGINE_TYPES_DRAWGROUPLIST_H
#define ENGINE_TYPES_DRAWGROUPLIST_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED

class Entity;

#include <Engine/Includes/Standard.h>
#include <Engine/Types/Entity.h>

class DrawGroupList {
public:
    int      EntityCount = 0;
    int      EntityCapacity = 0x1000;
    Entity** Entities = NULL;

            DrawGroupList();
    int    Add(Entity* obj);
    void    Remove(Entity* obj);
    void    Clear();
    void    Init();
    void    Dispose();
            ~DrawGroupList();
    int     Count();
};

#endif /* ENGINE_TYPES_DRAWGROUPLIST_H */
