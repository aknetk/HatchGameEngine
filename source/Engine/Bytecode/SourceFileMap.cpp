#if INTERFACE
#include <Engine/Hashing/CombinedHash.h>
#include <Engine/Includes/HashMap.h>
class SourceFileMap {
public:
    static bool             Initialized;
    static HashMap<Uint32>* Checksums;
};
#endif

#include <Engine/Bytecode/SourceFileMap.h>

#include <Engine/Bytecode/BytecodeObjectManager.h>
#include <Engine/Bytecode/Compiler.h>
#include <Engine/Diagnostics/Log.h>
#include <Engine/Filesystem/File.h>
#include <Engine/Filesystem/Directory.h>
#include <Engine/Hashing/FNV1A.h>

bool             SourceFileMap::Initialized = false;
HashMap<Uint32>* SourceFileMap::Checksums = NULL;

PUBLIC STATIC void SourceFileMap::CheckInit() {
    if (SourceFileMap::Initialized) return;

    if (SourceFileMap::Checksums == NULL) {
        SourceFileMap::Checksums = new HashMap<Uint32>(CombinedHash::EncryptString, 16);
    }

    #ifdef DEBUG

    if (File::Exists("SourceFileMap.bin")) {
        char*  bytes;
        size_t len = File::ReadAllBytes("SourceFileMap.bin", &bytes);
        SourceFileMap::Checksums->FromBytes((Uint8*)bytes, len / (sizeof(Uint32) + sizeof(Uint32)));
        Memory::Free(bytes);
    }

    #endif

    SourceFileMap::Initialized = true;
}
PUBLIC STATIC void SourceFileMap::CheckForUpdate() {
    SourceFileMap::CheckInit();

    #ifdef DEBUG

    bool anyChanges = false;

    vector<char*> list = Directory::GetFiles("Scripts", "*.obj", true);
    Directory::GetFiles(&list, "Scripts", "*.hsl", true);

    if (!Directory::Exists("Resources/Objects")) {
        Directory::Create("Resources/Objects");
    }

    for (size_t i = 0; i < list.size(); i++) {
        char* filename = strrchr(list[i], '/');
        Uint32 hash = 0;
        Uint32 newChecksum, oldChecksum;
        if (filename) {
            int dot = strlen(filename) - 4;
            filename[dot] = 0;
            hash = CombinedHash::EncryptString(filename + 1);
            filename[dot] = '.';
        }
        if (!hash) { Memory::Free(list[i]); continue; }

        newChecksum = 0;
        oldChecksum = 0;

        char*  source;
        File::ReadAllBytes(list[i], &source);
        newChecksum = FNV1A::EncryptString(source);

        Memory::Track(source, "SourceFileMap::SourceText");

        if (SourceFileMap::Checksums->Exists(hash)) {
            oldChecksum = SourceFileMap::Checksums->Get(hash);
        }
        anyChanges |= (newChecksum != oldChecksum);

        bool freeTokens = true;

        char outFile[256];
        sprintf(outFile, "Resources/Objects/%08X.ibc", hash);
        // If changed, then compile.
        // bool compiled = false;
        if (newChecksum != oldChecksum || !File::Exists(outFile) || !freeTokens) {
            Compiler::Init();
            Compiler* compiler = new Compiler;
            compiler->Compile(list[i], source, outFile);
            delete compiler;
            Compiler::Dispose(freeTokens);
            // compiled = true;
        }
        if (freeTokens)
            Memory::Free(source);

        // Log::Print(Log::LOG_INFO, "List: %s (%08X) (old: %08X, new: %08X) %d", list[i], hash, oldChecksum, newChecksum, compiled);

        SourceFileMap::Checksums->Put(hash, newChecksum);

        Memory::Free(list[i]);
    }

    if (anyChanges) {
        Uint8* data = SourceFileMap::Checksums->GetBytes(true);
        File::WriteAllBytes("SourceFileMap.bin", (char*)data, SourceFileMap::Checksums->Count * (sizeof(Uint32) + sizeof(Uint32)));
        Memory::Free(data);
    }

    #endif
}

PUBLIC STATIC void SourceFileMap::Dispose() {
    if (SourceFileMap::Checksums)
        delete SourceFileMap::Checksums;

    SourceFileMap::Initialized = false;
    SourceFileMap::Checksums = NULL;
}
