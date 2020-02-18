#ifndef SOURCEFILEMAP_H
#define SOURCEFILEMAP_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL


#include <Engine/Hashing/CombinedHash.h>
#include <Engine/Includes/HashMap.h>

class SourceFileMap {
public:
    static bool             Initialized;
    static HashMap<Uint32>* Checksums;

    static void CheckInit();
    static void CheckForUpdate();
    static void Dispose();
};

#endif /* SOURCEFILEMAP_H */
