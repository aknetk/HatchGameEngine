#if INTERFACE
class RemoteDebug {
public:
    static bool        Initialized;
    static bool        UsingRemoteDebug;
};
#endif

#include <Engine/Includes/Standard.h>
#include <Engine/Diagnostics/RemoteDebug.h>

bool        RemoteDebug::Initialized = false;
bool        RemoteDebug::UsingRemoteDebug = false;

PUBLIC STATIC void RemoteDebug::Init() {
    if (RemoteDebug::Initialized)
        return;

    RemoteDebug::Initialized = true;
}
