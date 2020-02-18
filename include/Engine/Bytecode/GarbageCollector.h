#ifndef GARBAGECOLLECTOR_H
#define GARBAGECOLLECTOR_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Bytecode/Types.h>
#include <Engine/Includes/HashMap.h>

class GarbageCollector {
public:
    static vector<Obj*> GrayList;
    static Obj*         RootObject;
    static bool Print;

    static void Collect();
    static void GrayValue(VMValue value);
    static void GrayObject(void* obj);
    static void GrayHashMapItem(Uint32, VMValue value);
    static void GrayHashMap(void* pointer);
    static void BlackenObject(Obj* object);
    static void RemoveWhiteHashMapItem(Uint32, VMValue value);
    static void RemoveWhiteHashMap(void* pointer);
};

#endif /* GARBAGECOLLECTOR_H */
