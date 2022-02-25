#ifndef ENGINE_TYPES_OBJECTLIST_H
#define ENGINE_TYPES_OBJECTLIST_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED

class Entity;
class Entity;
class Entity;

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
    double AverageUpdateTime = 0.0;
    double AverageUpdateItemCount = 0;
    double AverageUpdateEarlyTime = 0.0;
    double AverageUpdateEarlyItemCount = 0;
    double AverageUpdateLateTime = 0.0;
    double AverageUpdateLateItemCount = 0;
    double AverageRenderTime = 0.0;
    double AverageRenderItemCount = 0;
    Entity* (*SpawnFunction)() = NULL;

    void    Add(Entity* obj);
    void    Remove(Entity* obj);
    void    Clear();
    void    ReserveStatic(int count);
    Entity* AddStatic(Entity* obj);
    Entity* AddDynamic(Entity* obj);
    bool    RemoveDynamic(Entity* obj);
    Entity* GetNth(int n);
    Entity* GetClosest(int x, int y);
    void    Dispose();
            ~ObjectList();
    int     Count();
};

#endif /* ENGINE_TYPES_OBJECTLIST_H */
