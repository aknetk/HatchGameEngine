#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Includes/Standard.h>
#include <Engine/Includes/HashMap.h>

class ResourceManager {
public:
    static bool      UsingDataFolder;

    static void   PrefixResourcePath(char* out, const char* path);
    static void   PrefixParentPath(char* out, const char* path);
    static void   Init(const char* filename);
    static void   Load(const char* filename);
    static bool   LoadResource(const char* filename, Uint8** out, size_t* size);
    static bool   ResourceExists(const char* filename);
    static void   Dispose();
};

#endif /* RESOURCEMANAGER_H */
