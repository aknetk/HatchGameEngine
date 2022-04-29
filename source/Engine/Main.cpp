#include <Engine/Includes/Standard.h>
#include <Engine/Application.h>
#include <Engine/Diagnostics/Log.h>

#ifdef WIN32
    #include <windows.h>
#endif

int main(int argc, char* args[]) {
    #if SWITCH
        Log::Init();
        socketInitializeDefault();
        nxlinkStdio();
        #if defined(SWITCH_ROMFS)
        romfsInit();
        #endif
        // pcvInitialize(); 
        // pcvSetClockRate(PcvModule_CpuBus, 1581000000); // normal: 1020000000, overclock: 1581000000, strong overclock: 1785000000
    #endif

    #if WIN32
    AllocConsole();
    freopen_s((FILE **)stdin, "CONIN$", "w", stdin);
    freopen_s((FILE **)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE **)stderr, "CONOUT$", "w", stderr);
    #endif

    Application::Run(argc, args);

    #if WIN32
    FreeConsole();
    #endif

    #if SWITCH
        // pcvSetClockRate(PcvModule_CpuBus, 1020000000);
        // pcvExit();
        #if defined(SWITCH_ROMFS)
        romfsExit();
        #endif
        socketExit();
    #endif
    return 0;
}
