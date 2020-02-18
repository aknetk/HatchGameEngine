#ifndef OBJECTLIST_H
#define OBJECTLIST_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL

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

#endif /* OBJECTLIST_H */
