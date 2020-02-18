#if INTERFACE
#include <Engine/Includes/Standard.h>
#include <Engine/Application.h>
#include <Engine/Graphics.h>

#include <Engine/ResourceTypes/ResourceType.h>

#include <Engine/Types/EntityTypes.h>
#include <Engine/Includes/HashMap.h>
#include <Engine/Types/ObjectList.h>
#include <Engine/Scene/SceneLayer.h>
#include <Engine/Scene/TileConfig.h>

#include <Engine/Rendering/GL/GLShader.h>
#include <Engine/Scene/View.h>

need_t Entity;

class Scene {
public:
    static Uint32                BackgroundColor;

    static int                   FadeTimer;
    static int                   FadeTimerMax;
    static int                   FadeMax;
    static bool                  FadeIn;

    static HashMap<ObjectList*>* ObjectLists;
    static HashMap<ObjectList*>* ObjectRegistries;

    static int                   StaticObjectCount;
    static Entity*               StaticObjectFirst;
    static Entity*               StaticObjectLast;
    static int                   DynamicObjectCount;
    static Entity*               DynamicObjectFirst;
    static Entity*               DynamicObjectLast;

    static int                   PriorityPerLayer;
    static vector<Entity*>*      PriorityLists;

    static ISprite*              TileSprite;
    static Uint16                EmptyTile;

    static float                 CameraX;
    static float                 CameraY;
    static vector<SceneLayer>    Layers;
    static bool                  AnyLayerTileChange;

    static int                   TileCount;
    static int                   TileSize;
    static TileConfig*           TileCfgA;
    static TileConfig*           TileCfgB;
    static Uint32*               ExtraPalettes;

    static HashMap<ISprite*>*    SpriteMap;

    static vector<ResourceType*> SpriteList;
    static vector<ResourceType*> ImageList;
    static vector<ResourceType*> SoundList;
    static vector<ResourceType*> MusicList;
    static vector<GLShader*> ShaderList;
    static vector<ResourceType*> ModelList;
    static vector<ResourceType*> MediaList;

    static int                   Frame;
    static bool                  Paused;
    static int                   MainLayer;

    static View                  Views[8];
    static int                   ViewCurrent;

    static char                  NextScene[256];
    static char                  CurrentScene[256];
    static bool                  DoRestart;
};
#endif

#include <Engine/Scene.h>

#include <Engine/Audio/AudioManager.h>
#include <Engine/Bytecode/BytecodeObject.h>
#include <Engine/Bytecode/BytecodeObjectManager.h>
#include <Engine/Bytecode/SourceFileMap.h>
#include <Engine/Diagnostics/Clock.h>
#include <Engine/Diagnostics/Log.h>
#include <Engine/Diagnostics/Memory.h>
#include <Engine/Filesystem/File.h>
#include <Engine/Hashing/CombinedHash.h>
#include <Engine/Hashing/CRC32.h>
#include <Engine/Hashing/FNV1A.h>
#include <Engine/Hashing/MD5.h>
#include <Engine/IO/FileStream.h>
#include <Engine/IO/MemoryStream.h>
#include <Engine/IO/ResourceStream.h>
#include <Engine/IO/Compression/ZLibStream.h>
#include <Engine/Rendering/GL/GLRenderer.h>
#include <Engine/ResourceTypes/SceneFormats/RSDKSceneReader.h>
#include <Engine/ResourceTypes/ISound.h>
#include <Engine/ResourceTypes/ResourceManager.h>
#include <Engine/Scene/TiledMapReader.h>
#include <Engine/TextFormats/XML/XMLParser.h>
#include <Engine/Types/EntityTypes.h>
#include <Engine/Includes/HashMap.h>
#include <Engine/Types/ObjectList.h>

#define FIRSTGID 1
#define TILE_FLIPX_MASK 0x80000000U
#define TILE_FLIPY_MASK 0x40000000U
// #define TILE_DIAGO_MASK 0x20000000U
#define TILE_COLLA_MASK 0x30000000U
#define TILE_COLLB_MASK 0x0C000000U
#define TILE_COLLC_MASK 0x03000000U
#define TILE_IDENT_MASK 0x00FFFFFFU

// Layering variables
vector<SceneLayer>    Scene::Layers;
bool                  Scene::AnyLayerTileChange = false;
int                   Scene::MainLayer = 0;
int                   Scene::PriorityPerLayer = 2; // NOTE: Replace this with a non-pot number you whackjob
vector<Entity*>*      Scene::PriorityLists = NULL;

// Rendering variables
Uint32                Scene::BackgroundColor = 0x000000;
int                   Scene::FadeTimer = -1;
int                   Scene::FadeTimerMax = 1;
int                   Scene::FadeMax = 0xFF;
bool                  Scene::FadeIn = false;

// Object variables
HashMap<ObjectList*>* Scene::ObjectLists = NULL;
HashMap<ObjectList*>* Scene::ObjectRegistries = NULL;

int                   Scene::StaticObjectCount = 0;
Entity*               Scene::StaticObjectFirst = NULL;
Entity*               Scene::StaticObjectLast = NULL;
int                   Scene::DynamicObjectCount = 0;
Entity*               Scene::DynamicObjectFirst = NULL;
Entity*               Scene::DynamicObjectLast = NULL;

// Tile variables
ISprite*              Scene::TileSprite = NULL;
int                   Scene::TileCount = 0;
int                   Scene::TileSize = 16;
TileConfig*           Scene::TileCfgA = NULL;
TileConfig*           Scene::TileCfgB = NULL;
Uint16                Scene::EmptyTile = 0x000;
Uint32*               Scene::ExtraPalettes = NULL;

// ??? variables
int                   Scene::Frame = 0;
bool                  Scene::Paused = false;
float                 Scene::CameraX = 0.0f;
float                 Scene::CameraY = 0.0f;
View                  Scene::Views[8];
int                   Scene::ViewCurrent = 0;

char                  Scene::NextScene[256];
char                  Scene::CurrentScene[256];
bool                  Scene::DoRestart = false;

// Resource managing variables
HashMap<ISprite*>*    Scene::SpriteMap = NULL;
vector<ResourceType*> Scene::SpriteList;
vector<ResourceType*> Scene::ImageList;
vector<ResourceType*> Scene::SoundList;
vector<ResourceType*> Scene::MusicList;
vector<GLShader*>     Scene::ShaderList;
vector<ResourceType*> Scene::ModelList;
vector<ResourceType*> Scene::MediaList;

bool                  DEV_NoTiles = false;
bool                  DEV_NoObjectRender = false;
const char*           DEBUG_lastTileColFilename = NULL;

void _ObjectList_RemoveNonPersistentDynamicFromLists(Uint32, ObjectList* list) {
    // NOTE: We don't use any list clearing functions so that
    //   we can support persistent objects later.
    for (Entity* ent = Scene::DynamicObjectFirst, *next; ent; ent = next) {
        // Store the "next" so that when/if the current is removed,
        // it can still be used to point at the end of the loop.
        next = ent->NextEntity;

        // Remove the current
        if (ent->List == list)
            list->Remove(ent);
    }
}
void _ObjectList_Clear(Uint32, ObjectList* list) {
    list->Clear();
}
void _UpdateObject(Entity* ent) {
    if (Scene::Paused && ent->Pauseable)
        return;

    if (!ent->Active)
        return;

    // int oldPriority = ent->Priority;

    ent->Update();

    int oldPriority = ent->PriorityOld;
    int maxPriority = (Scene::Layers.size() << Scene::PriorityPerLayer) - 1;
    // HACK: This needs to error
    // if (ent->Priority < 0 || (ent->Priority >= (Scene::Layers.size() << Scene::PriorityPerLayer)))
    //     return;
    if (ent->Priority < 0)
        ent->Priority = 0;
    if (ent->Priority > maxPriority)
        ent->Priority = maxPriority;

    // If hasn't been put in a list yet.
    if (ent->PriorityListIndex == -1) {
        ent->PriorityListIndex = Scene::PriorityLists[ent->Priority].size();
        Scene::PriorityLists[ent->Priority].push_back(ent);
    }
    // If Priority is changed.
    else if (ent->Priority != oldPriority) {
        // Remove entry in old list.
        Scene::PriorityLists[oldPriority][ent->PriorityListIndex] = NULL;
        int a = -1;
        for (size_t p = 0, pSz = Scene::PriorityLists[ent->Priority].size(); p < pSz; p++) {
            if (!Scene::PriorityLists[ent->Priority][p]) {
                Scene::PriorityLists[ent->Priority][p] = ent;
                a = (int)p;
                break;
            }
        }
        if (a < 0) {
            a = (int)Scene::PriorityLists[ent->Priority].size();
            Scene::PriorityLists[ent->Priority].push_back(ent);
        }

        ent->PriorityListIndex = a;
    }
    ent->PriorityOld = ent->Priority;
}

// Double linked-list functions
PUBLIC STATIC void Scene::Add(Entity** first, Entity** last, int* count, Entity* obj) {
    // Set "prev" of obj to last
    obj->PrevEntity = (*last);
    obj->NextEntity = NULL;

    // If the last exists, set that ones "next" to obj
    if ((*last))
        (*last)->NextEntity = obj;

    // Set obj as the first if there is not one
    if (!(*first))
        (*first) = obj;

    (*last) = obj;

    (*count)++;

    // Add to proper list
    if (!obj->List) {
        Log::Print(Log::LOG_IMPORTANT, "obj->List = NULL");
        exit(0);
    }
    obj->List->Add(obj);
}
PUBLIC STATIC void Scene::Remove(Entity** first, Entity** last, int* count, Entity* obj) {
    if (obj == NULL) return;

    if ((*first) == obj)
        (*first) = obj->NextEntity;

    if ((*last) == obj)
        (*last) = obj->PrevEntity;

    if (obj->PrevEntity)
        obj->PrevEntity->NextEntity = obj->NextEntity;

    if (obj->NextEntity)
        obj->NextEntity->PrevEntity = obj->PrevEntity;

    (*count)--;

    // Remove from proper list
    if (!obj->List) {
        Log::Print(Log::LOG_IMPORTANT, "obj->List = NULL");
        exit(0);
    }
    obj->List->Remove(obj);
}
PUBLIC STATIC void Scene::Clear(Entity** first, Entity** last, int* count) {
    (*first) = NULL;
    (*last) = NULL;
    (*count) = 0;
}

// Object management
PUBLIC STATIC void Scene::AddStatic(ObjectList* objectList, Entity* obj) {
    Scene::Add(&Scene::StaticObjectFirst, &Scene::StaticObjectLast, &Scene::StaticObjectCount, obj);
}
PUBLIC STATIC void Scene::AddDynamic(ObjectList* objectList, Entity* obj) {
    Scene::Add(&Scene::DynamicObjectFirst, &Scene::DynamicObjectLast, &Scene::DynamicObjectCount, obj);
}

PUBLIC STATIC void Scene::OnEvent(Uint32 event) {
    switch (event) {
        case SDL_APP_TERMINATING:
        case SDL_APP_LOWMEMORY:
        case SDL_APP_WILLENTERBACKGROUND:
        case SDL_APP_DIDENTERBACKGROUND:
        case SDL_APP_WILLENTERFOREGROUND:
        case SDL_APP_DIDENTERFOREGROUND:
            // Call "WindowFocusLost" event on all objects
            break;
        default:
            break;
    }
}

PUBLIC STATIC void Scene::Init() {
    Scene::NextScene[0] = '\0';
    Scene::CurrentScene[0] = '\0';

    SourceFileMap::CheckForUpdate();

    BytecodeObjectManager::Init();
    BytecodeObjectManager::ResetStack();
    BytecodeObjectManager::LinkStandardLibrary();

    Application::Settings->GetBool("dev", "notiles", &DEV_NoTiles);
    Application::Settings->GetBool("dev", "noobjectrender", &DEV_NoObjectRender);

    Scene::ViewCurrent = 0;
    Scene::Views[0].Active = true;
    Scene::Views[0].Width = 640;
    Scene::Views[0].Height = 480;
    Scene::Views[0].FOV = 45.0f;
    Scene::Views[0].UsePerspective = false;
    Scene::Views[0].DrawTarget = Graphics::CreateTexture(SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, Scene::Views[0].Width, Scene::Views[0].Height);
    Scene::Views[0].UseDrawTarget = true;
    Scene::Views[0].ProjectionMatrix = Matrix4x4::Create();
    Scene::Views[0].BaseProjectionMatrix = Matrix4x4::Create();

    for (int i = 1; i < 8; i++) {
        Scene::Views[i].Active = false;
        Scene::Views[i].Width = 640;
        Scene::Views[i].Height = 480;
        Scene::Views[i].FOV = 45.0f;
        Scene::Views[i].UsePerspective = false;
        Scene::Views[i].DrawTarget = Graphics::CreateTexture(SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, Scene::Views[i].Width, Scene::Views[i].Height);
        Scene::Views[i].UseDrawTarget = true;
        Scene::Views[i].ProjectionMatrix = Matrix4x4::Create();
        Scene::Views[i].BaseProjectionMatrix = Matrix4x4::Create();
    }
}
PUBLIC STATIC void Scene::Restart() {
    Scene::ViewCurrent = 0;

    View* currentView = &Scene::Views[Scene::ViewCurrent];
    currentView->X = 0.0f;
    currentView->Y = 0.0f;
    currentView->Z = 0.0f;
    Scene::Frame = 0;
    Scene::Paused = false;

    // Deactivate extra views
    for (int i = 1; i < 8; i++) {
        Scene::Views[i].Active = false;
    }

    int layerSize = (int)Layers.size();
    if (Scene::AnyLayerTileChange) {
        // Copy backup tiles into main tiles
        for (int l = 0; l < layerSize; l++) {
            memcpy(Layers[l].Tiles, Layers[l].TilesBackup, Layers[l].Width * Layers[l].Height * sizeof(Uint32));
        }
        Scene::AnyLayerTileChange = false;
    }
    layerSize <<= PriorityPerLayer;
    for (int l = 0; l < layerSize; l++) {
        Scene::PriorityLists[l].clear();
    }

    // Remove all non-persistent objects from lists
	if (Scene::ObjectLists)
		Scene::ObjectLists->ForAll(_ObjectList_RemoveNonPersistentDynamicFromLists);

    // Remove all non-persistent objects from lists
	if (Scene::ObjectRegistries)
		Scene::ObjectRegistries->ForAll(_ObjectList_Clear);

    // Dispose of all dynamic objects
    for (Entity* ent = Scene::DynamicObjectFirst, *next; ent; ent = next) {
        // Store the "next" so that when/if the current is removed,
        // it can still be used to point at the end of the loop.
        next = ent->NextEntity;

        // Dispose of object
        ent->Dispose();
    }

    Scene::Clear(&Scene::DynamicObjectFirst, &Scene::DynamicObjectLast, &Scene::DynamicObjectCount);

    // Run "Create" on all objects
    for (Entity* ent = Scene::StaticObjectFirst, *next; ent; ent = next) {
        // Store the "next" so that when/if the current is removed,
        // it can still be used to point at the end of the loop.
        next = ent->NextEntity;

        // Execute whatever on object
        ent->X = ent->InitialX;
        ent->Y = ent->InitialY;
        ent->Create();
    }
    // NOTE: We don't need to do dynamic objects here since we cleared out the list.
    // TODO: We should do this for any persistent dynamic objects.

    BytecodeObjectManager::ResetStack();
    BytecodeObjectManager::RequestGarbageCollection();
}

PUBLIC STATIC void Scene::LoadScene(const char* filename) {
    /// Reset everything
    // Clear lists
    if (Scene::ObjectLists) {
        Scene::ObjectLists->ForAll(_ObjectList_Clear);
    }
    if (Scene::ObjectRegistries) {
        Scene::ObjectRegistries->ForAll(_ObjectList_Clear);
    }

    // Clear and dispose of objects
    // TODO: Alter this for persistency
    for (Entity* ent = Scene::StaticObjectFirst, *next; ent; ent = next) {
        next = ent->NextEntity;
        ent->Dispose();
    }
    Scene::Clear(&Scene::StaticObjectFirst, &Scene::StaticObjectLast, &Scene::StaticObjectCount);

    for (Entity* ent = Scene::DynamicObjectFirst, *next; ent; ent = next) {
        next = ent->NextEntity;
        ent->Dispose();
    }
    Scene::Clear(&Scene::DynamicObjectFirst, &Scene::DynamicObjectLast, &Scene::DynamicObjectCount);

    // Clear PriorityLists
    int layerSize = Scene::Layers.size() << PriorityPerLayer;
    for (int l = 0; l < layerSize; l++) {
        Scene::PriorityLists[l].clear();
    }

    // Dispose of layers
    for (size_t i = 0; i < Scene::Layers.size(); i++) {
        Scene::Layers[i].Dispose();
    }
    Scene::Layers.clear();

    // Request garbage collect
    BytecodeObjectManager::ResetStack();
    BytecodeObjectManager::RequestGarbageCollection();
    ////

    char pathParent[256];
    strcpy(pathParent, filename);
    for (char* i = pathParent + strlen(pathParent); i >= pathParent; i--) {
        if (*i == '/') {
            *++i = 0;
            break;
        }
    }

    strcpy(Scene::CurrentScene, filename);
    Scene::CurrentScene[strlen(filename)] = 0;

	if (Scene::TileSprite) {
		Scene::TileSprite->Dispose();
		Scene::TileSprite = NULL;
	}

    Log::Print(Log::LOG_INFO, "Starting scene \"%s\"...", filename);
    if (strstr(filename, ".bin"))
        RSDKSceneReader::Read(filename, pathParent);
    else
        TiledMapReader::Read(filename, pathParent);

    #if 0
        String_StageConfig = "Unknown";
        String_SceneBin = "Unknown";
        String_TileConfig = "Unknown";
        String_TileSprite = NULL;

        HashMap<char*> ObjectHashes;

        // NOTE: We need to iterate through all usable objects names and load them
        // but since we don't have a pre-made binary for that
        // just use StageConfig for the list of object names
        ResourceStream* stageConfigReader;
        if ((stageConfigReader = ResourceStream::New(String_StageConfig))) {
            MemoryStream* memoryReader;
            if ((memoryReader = MemoryStream::New(stageConfigReader))) {
                do {
                    Uint32 magic = memoryReader->ReadUInt32();
                    if (magic != 0x474643)
                        break;

                    // int useGameObjects =
                        memoryReader->ReadByte();

                    int objectNameCount = memoryReader->ReadByte();
                    char* objectName;
                    Uint32 objectNameHash;
                    for (int i = 0; i < objectNameCount; i++) {
                        objectName = memoryReader->ReadHeaderedString();
                        objectNameHash = CombinedHash::EncryptString(objectName);

                        ObjectHashes.Put(objectNameHash, objectName);

                        BytecodeObjectManager::GetSpawnFunction(objectNameHash, objectName);
                    }

                    int paletteCount = 8;
                    if (Scene::ExtraPalettes == NULL)
                        Scene::ExtraPalettes = (Uint32*)Memory::TrackedCalloc("Scene::ExtraPalettes", sizeof(Uint32) * 0x100, 8);

                    for (int i = 0; i < paletteCount; i++) {
                        // Palette Set
                        int bitmap = memoryReader->ReadUInt16();
                        for (int col = 0; col < 16; col++) {
                            if ((bitmap & (1 << col)) != 0) {
                                for (int d = 0; d < 16; d++) {
                                    Uint8 Color[3];
                                    memoryReader->ReadBytes(Color, 3);
                                    Scene::ExtraPalettes[(i << 8) | (col << 4) | d] = 0xFF000000U | Color[0] << 16 | Color[1] << 8 | Color[2];
                                }
                            }
                        }
                    }

                    int wavConfigCount = memoryReader->ReadByte();
                    for (int i = 0; i < wavConfigCount; i++) {
                        Memory::Free(memoryReader->ReadHeaderedString());
                        // Max Concurrent Play
                        memoryReader->ReadByte();
                    }
                } while (false);

                memoryReader->Close();
            }
            stageConfigReader->Close();
        }

        ResourceStream* sceneBinReader;
        if ((sceneBinReader = ResourceStream::New(String_SceneBin))) {
            MemoryStream* memoryReader;
            if ((memoryReader = MemoryStream::New(sceneBinReader))) {
                memoryReader->ReadUInt32BE(); // Magic
                memoryReader->Skip(16); // 16 bytes
                memoryReader->Skip(memoryReader->ReadByte()); // RSDKString

            	memoryReader->ReadByte();

                double ticks;

                ticks = Clock::GetTicks();

                int layerCount = memoryReader->ReadByte();
                Layers.resize(layerCount);
                for (int i = 0; i < layerCount; i++) {
                    memoryReader->ReadByte(); // Ignored Byte

                    char* Name = memoryReader->ReadHeaderedString();
                    memoryReader->ReadByte(); // int   IsScrollingVertical = memoryReader->ReadByte() == 1 ? true : false;
                    memoryReader->ReadByte(); // int   Flags = memoryReader->ReadByte();
                    int   Width = (int)memoryReader->ReadUInt16();
                    int   Height = (int)memoryReader->ReadUInt16();

                    SceneLayer layer(Width, Height);

                    memset(layer.Name, 0, 50);
                    strcpy(layer.Name, Name);
                    Memory::Free(Name);

                    if (MainLayer == 0 && layer.Name[0] == 'F' && layer.Name[1] == 'G')
                        MainLayer = i;

                    layer.RelativeY = memoryReader->ReadUInt16();
                    layer.ConstantY = (short)memoryReader->ReadUInt16();

                    layer.Flags = SceneLayer::FLAGS_COLLIDEABLE;

                    layer.ScrollInfoCount = (int)memoryReader->ReadUInt16();
                    layer.ScrollInfos = (ScrollingInfo*)Memory::Malloc(layer.ScrollInfoCount * sizeof(ScrollingInfo));
                    for (int g = 0; g < layer.ScrollInfoCount; g++) {
                        layer.ScrollInfos[g].RelativeX = memoryReader->ReadUInt16();
                        layer.ScrollInfos[g].ConstantX = memoryReader->ReadUInt16();

                        memoryReader->ReadByte();
                        memoryReader->ReadByte();
                    }

                    memoryReader->ReadCompressed(layer.ScrollIndexes);
                    memoryReader->ReadCompressed(layer.Tiles);

        			Log::Print(Log::LOG_VERBOSE, "Layer %d (%s): Width (%d) Height (%d)", i, layer.Name, layer.Width, layer.Height);

                    Layers[i] = layer;
                }

                PriorityLists = (vector<Entity*>*)Memory::TrackedCalloc("Scene::PriorityLists", layerCount * (1 << PriorityPerLayer), sizeof(vector<Entity*>));

                ticks = Clock::GetTicks() - ticks;
                Log::Print(Log::LOG_VERBOSE, "Scene Layer load took %.3f milliseconds.", ticks);

                ticks = Clock::GetTicks();

                int objectDefinitionCount = memoryReader->ReadByte();
                Log::Print(Log::LOG_VERBOSE, "Object Definition Count: %d", objectDefinitionCount);

                int roundedUpPow2 = objectDefinitionCount;

                roundedUpPow2--;
                roundedUpPow2 |= roundedUpPow2 >> 1;
                roundedUpPow2 |= roundedUpPow2 >> 2;
                roundedUpPow2 |= roundedUpPow2 >> 4;
                roundedUpPow2 |= roundedUpPow2 >> 16;
                roundedUpPow2++;

                if (Scene::ObjectLists == NULL) {
                    Scene::ObjectLists = new HashMap<ObjectList*>(CombinedHash::EncryptString, roundedUpPow2);
                    Scene::ObjectRegistries = new HashMap<ObjectList*>(CombinedHash::EncryptString, 16);
                }

                int    objectListTotalCount = 0;

                Uint8  hashTemp[16];
                Uint32 objectNameHash;
                int    argumentTypes[0x10];
                Uint32 argumentHashes[0x10];
                int    argumentCount;
                int    entityCount;

                for (int i = 0; i < objectDefinitionCount; i++) {
                    memoryReader->ReadBytes(hashTemp, 16);
                    objectNameHash = CRC32::EncryptData(hashTemp, 16);

                    ObjectList* objectList = new ObjectList();
                    Scene::ObjectLists->Put(objectNameHash, objectList);

                    argumentCount = memoryReader->ReadByte();

                    argumentTypes[0] = 8;
                    argumentHashes[0] = 0x00000000U;
                    for (int a = 1; a < argumentCount; a++) {
                        memoryReader->ReadBytes(hashTemp, 16);
                        argumentTypes[a] = memoryReader->ReadByte();
                        argumentHashes[a] = CRC32::EncryptData(hashTemp, 16);
                    }

                    entityCount = memoryReader->ReadUInt16();

                    if (ObjectHashes.Exists(objectNameHash)) {
                        // Log::Print(Log::LOG_VERBOSE, "Object Hash: 0x%08XU Count: %*d Argument Count: %d (%s)", objectNameHash, 3, entityCount, argumentCount, ObjectHashes[objectNameHash]);
                    }
                    else {
                        // Log::Print(Log::LOG_VERBOSE, "Object Hash: 0x%08XU Count: %*d Argument Count: %d", objectNameHash, 3, entityCount, argumentCount);
                    }

                    /*
                    char filename[64];
                    ResourceStream* stream;
                    sprintf(filename, "Objects/%08X.ibc", objectNameHash);
                    if ((stream = ResourceStream::New(filename)) && ObjectHashes.Exists(objectNameHash)) {
                        // Check to see if bytecode hasn't already been loaded, and if so just point to that one
                        BytecodeObjectManager::Load(stream);
                        stream->Close();
                        BytecodeObjectManager::SetCurrentObjectHash(FNV1A::EncryptString(ObjectHashes.Get(objectNameHash)));

                        objectList->SpawnFunction = BytecodeObjectManager::SpawnFunction;
                    }
                    //*/
                    if (ObjectHashes.Exists(objectNameHash))
                        objectList->SpawnFunction = (Entity* (*)())BytecodeObjectManager::GetSpawnFunction(objectNameHash, ObjectHashes.Get(objectNameHash));

                    if (objectList->SpawnFunction) {
                        objectListTotalCount += entityCount;
                        // Objects.reserve(objectListTotalCount);
                        // objectList->ReserveStatic(entityCount);
                    }

                    for (int n = 0; n < entityCount; n++) {
                        memoryReader->ReadUInt16(); // int SlotID = memoryReader->ReadUInt16();

                        Uint32 X = memoryReader->ReadUInt32();
                        Uint32 Y = memoryReader->ReadUInt32();

                        if (objectList->SpawnFunction && n == 0) {
                            Entity* obj = objectList->SpawnFunction();
                            obj->X = (X / 65536.f);
                            obj->Y = (Y / 65536.f);
                            obj->InitialX = X;
                            obj->InitialY = Y;
                            obj->List = objectList;
                            Scene::AddStatic(obj);

                            for (int a = 1; a < argumentCount; a++) {
                                AttributeValue attr;
                                switch (argumentTypes[a]) {
                                    case 0x0: attr.Value.asUint8  = memoryReader->ReadByte(); break;
                                    case 0x1: attr.Value.asUint16 = memoryReader->ReadUInt16(); break;
                                    case 0x2: attr.Value.asUint32 = memoryReader->ReadUInt32(); break;
                                    case 0x3: attr.Value.asInt8   = memoryReader->ReadByte(); break;
                                    case 0x4: attr.Value.asInt16  = memoryReader->ReadInt16(); break;
                                    case 0x5: attr.Value.asInt32  = memoryReader->ReadInt32(); break;
                                    // Var
                                    case 0x6: attr.Value.asInt32  = memoryReader->ReadInt32(); break;
                                    // Bool
                                    case 0x7: attr.Value.asBool   = (bool)memoryReader->ReadUInt32(); break;
                                    // String
                                    case 0x8:
                                        attr.Value.asString.Length = memoryReader->ReadUInt16();
                                        attr.Value.asString.Data = (char*)Memory::TrackedMalloc("Args::asString::Data", attr.Value.asString.Length + 1);
                                        for (int c = 0; c < attr.Value.asString.Length; c++)
                                            attr.Value.asString.Data[c] = (char)(Uint8)memoryReader->ReadUInt16();
                                        attr.Value.asString.Data[attr.Value.asString.Length] = 0;
                                        break;
                                    // Position
                                    case 0x9:
                                        attr.Value.asUint32 = memoryReader->ReadUInt32();
                						memoryReader->ReadUInt32();
                                        break;
                                    // Color
                                    case 0xB: attr.Value.asUint32 = memoryReader->ReadUInt32(); break;
                                }
                                // obj->Attributes.Put(argumentHashes[a], attr);
                            }
                        }
                        else {
                            int len;
                            for (int a = 1; a < argumentCount; a++) {
                                switch (argumentTypes[a]) {
                                    case 0x0: memoryReader->Skip(1); break;
                                    case 0x1: memoryReader->Skip(2); break;
                                    case 0x2: memoryReader->Skip(4); break;
                                    case 0x3: memoryReader->Skip(1); break;
                                    case 0x4: memoryReader->Skip(2); break;
                                    case 0x5: memoryReader->Skip(4); break;
                                    // Var
                                    case 0x6: memoryReader->Skip(4); break;
                                    // Bool
                                    case 0x7: memoryReader->Skip(4); break;
                                    // String
                                    case 0x8:
                                        len = memoryReader->ReadUInt16();
                                        memoryReader->Skip(len << 1);
                                        break;
                                    // Position
                                    case 0x9:
                                        memoryReader->Skip(8);
                                        break;
                                    // Color
                                    case 0xB: memoryReader->Skip(4); break;
                                }
                            }
                        }
                    }
                }

                ticks = Clock::GetTicks() - ticks;
                Log::Print(Log::LOG_VERBOSE, "Scene Object load took %.3f milliseconds.", ticks);

                memoryReader->Close();
            }
            sceneBinReader->Close();
        }

        ResourceStream* tileConfigReader;
        if ((tileConfigReader = ResourceStream::New(String_TileConfig))) {
            if (Scene::TileCfgA == NULL) {
        		Scene::TileCfgA = (TileConfig*)Memory::TrackedCalloc("Scene::TileCfgA", 0x800, sizeof(TileConfig));
        		Scene::TileCfgB = Scene::TileCfgA + 0x400 * sizeof(TileConfig);

                for (int i = 0; i < 0x800; i++) {
                    Scene::TileCfgA[i].Collision = (Uint8*)Memory::Calloc(1, Scene::TileSize);
                    Scene::TileCfgA[i].HasCollision = (Uint8*)Memory::Calloc(1, Scene::TileSize);
                }
            }
            MemoryStream* memoryReader;
            if ((memoryReader = MemoryStream::New(tileConfigReader))) {
                memoryReader->ReadUInt32(); // Magic 0x4C4954
                // NOTE: This won't work because pointers now
                // memoryReader->ReadCompressed(Scene::TileCfgA);
                memoryReader->Close();
            }
            tileConfigReader->Close();
        }

        if (String_TileSprite && false) {
            TileSprite = new ISprite(String_TileSprite);
            TileSprite->ReserveAnimationCount(1);
            TileSprite->AddAnimation("TileSprite", 0, 0, 0x400);
            if (TileSprite->Width > 16) {
                for (int i = 0; i < 0x400; i++)
                    TileSprite->AddFrame(0, (i & 0x1F) << 4, (i >> 5) << 4, 16, 16, -8, -8);
            }
            else {
                for (int i = 0; i < 0x400; i++)
                    TileSprite->AddFrame(0, 0, i << 4, 16, 16, -8, -8);
            }
        }

        ObjectHashes.WithAll([](Uint32 hash, char* string) -> void {
            Memory::Free(string);
        });
    #endif

    if (Graphics::SupportsBatching && Scene::TileSprite) {
        struct tempVertex {
            float x;
            float y;
            float z;
            float u;
            float v;
        };
        float spriteW = Scene::TileSprite->Spritesheets[0]->Width;
        float spriteH = Scene::TileSprite->Spritesheets[0]->Height;
        float tileSize = (float)Scene::TileSize;
        for (size_t l = 0; l < Scene::Layers.size(); l++) {
            SceneLayer* layer = &Scene::Layers[l];
            int sz = layer->Width * layer->Height;
            if (!layer->Visible) continue;

            tempVertex* buffer = (tempVertex*)Memory::Malloc(sz * sizeof(tempVertex) * 6);

            int    vertexCount = 0;
            int    tSauce, tileID, flipX, flipY, tx, ty, srcx, srcy;
            float  left, right, top, bottom, posX, posY, posXW, posYH;
            for (int t = 0; t < sz; t++) {
				if ((layer->Tiles[t] & TILE_IDENT_MASK) == EmptyTile) continue;

                tSauce = layer->Tiles[t];

                tileID = (tSauce & TILE_IDENT_MASK) - 1;
                flipX  = tSauce & TILE_FLIPX_MASK;
                flipY  = tSauce & TILE_FLIPY_MASK;

                tx = t % layer->Width;
                ty = t / layer->Width;
                srcx = Scene::TileSprite->Animations[0].Frames[tileID].X;
                srcy = Scene::TileSprite->Animations[0].Frames[tileID].Y;

                if (flipX) {
                    right = (srcx) / spriteW;
                    left  = (srcx + tileSize) / spriteW;
                }
                else {
                    left  = (srcx) / spriteW;
                    right = (srcx + tileSize) / spriteW;
                }
                if (flipY) {
                    bottom = (srcy) / spriteH;
                    top    = (srcy + tileSize) / spriteH;
                }
                else {
                    top    = (srcy) / spriteH;
                    bottom = (srcy + tileSize) / spriteH;
                }

                posX = tx * tileSize;
                posY = ty * tileSize;
                posXW = posX + tileSize;
                posYH = posY + tileSize;

                buffer[vertexCount++] = tempVertex { posX, posY, 0.0f, left, top };
                buffer[vertexCount++] = tempVertex { posX, posYH, 0.0f, left, bottom };
                buffer[vertexCount++] = tempVertex { posXW, posY, 0.0f, right, top };

                buffer[vertexCount++] = tempVertex { posXW, posY, 0.0f, right, top };
                buffer[vertexCount++] = tempVertex { posX, posYH, 0.0f, left, bottom };
                buffer[vertexCount++] = tempVertex { posXW, posYH, 0.0f, right, bottom };
            }

            layer->BufferID = GLRenderer::CreateTexturedShapeBuffer((float*)buffer, vertexCount);
            // layer->BufferID = D3DRenderer::CreateTexturedShapeBuffer((float**)&buffer, vertexCount);
            layer->VertexCount = vertexCount;

            Memory::Free(buffer);
        }
    }
}
PUBLIC STATIC void Scene::LoadTileCollisions(const char* filename) {
    Stream* tileColReader;
    if (!ResourceManager::ResourceExists(filename)) {
        Log::Print(Log::LOG_WARN, "Could not find tile collision file \"%s\"!", filename);
        return;
    }

    tileColReader = ResourceStream::New(filename);
    if (!tileColReader) {
        DEBUG_lastTileColFilename = NULL;
        return;
    }

    DEBUG_lastTileColFilename = filename;

    Uint32 magic = tileColReader->ReadUInt32();
    // RSDK TileConfig
    if (magic == 0x004C4954U) {
        Uint32 tileCount = 0x400;

        // Log::Print(Log::LOG_INFO, "filename: %s", filename);

        Uint8* tileInfo = (Uint8*)Memory::Calloc(1, (tileCount + 1) * 2 * 0x26);
        tileColReader->ReadCompressed(tileInfo);

        Scene::TileSize = 16;
        Scene::TileCount = tileCount + 1;
        if (Scene::TileCfgA == NULL) {
            Scene::TileCfgA = (TileConfig*)Memory::TrackedCalloc("Scene::TileCfgA", (Scene::TileCount << 1), sizeof(TileConfig));
            Scene::TileCfgB = Scene::TileCfgA + Scene::TileCount;

            for (Uint32 i = 0, iSz = (Uint32)(Scene::TileCount << 1); i < iSz; i++) {
                Scene::TileCfgA[i].Collision = (Uint8*)Memory::Calloc(1, Scene::TileSize);
                Scene::TileCfgA[i].HasCollision = (Uint8*)Memory::Calloc(1, Scene::TileSize);
            }
        }

        Uint8* line;
        for (Uint32 i = 0; i < tileCount; i++) {
            line = &tileInfo[(i - 1) * 0x26];
            if (i == 0)
                line = &tileInfo[i * 0x26];

            Scene::TileCfgA[i].IsCeiling = line[0x20];
            // Angles
            memcpy(&Scene::TileCfgA[i].Config[0], &line[0x21], 5);
            memcpy(&Scene::TileCfgA[i].Collision[0], &line[0x00], 0x10);
            memcpy(&Scene::TileCfgA[i].HasCollision[0], &line[0x10], 0x10);

            for (int t = 0; t < Scene::TileSize; t++) {
                if (Scene::TileCfgA[i].IsCeiling)
                    Scene::TileCfgA[i].Collision[t] ^= 0xF;
            }
        }
        for (Uint32 i = 0; i < tileCount; i++) {
            line = &tileInfo[(tileCount + (i - 1)) * 0x26];
            if (i == 0)
                line = &tileInfo[(tileCount + i) * 0x26];

            Scene::TileCfgB[i].IsCeiling = line[0x20];
            // Angles
            memcpy(&Scene::TileCfgB[i].Config[0], &line[0x21], 5);
            memcpy(&Scene::TileCfgB[i].Collision[0], &line[0x00], 0x10);
            memcpy(&Scene::TileCfgB[i].HasCollision[0], &line[0x10], 0x10);

            for (int t = 0; t < Scene::TileSize; t++) {
                if (Scene::TileCfgB[i].IsCeiling)
                    Scene::TileCfgB[i].Collision[t] ^= 0xF;
            }
        }

        Memory::Free(tileInfo);
    }
    else
    if (magic != 0x00000000U) {
        Uint32 tileCount = tileColReader->ReadUInt32();
        Uint8  tileSize  = tileColReader->ReadByte();
        tileColReader->ReadByte();
        tileColReader->ReadByte();
        tileColReader->ReadByte();
        tileColReader->ReadUInt32();

        Scene::TileCount = tileCount;
        if (Scene::TileCfgA == NULL) {
            Scene::TileSize = tileSize;
            Scene::TileCfgA = (TileConfig*)Memory::TrackedCalloc("Scene::TileCfgA", tileCount << 1, sizeof(TileConfig));
            Scene::TileCfgB = Scene::TileCfgA + tileCount;

            for (Uint32 i = 0; i < (tileCount << 1); i++) {
                Scene::TileCfgA[i].Collision = (Uint8*)Memory::Calloc(1, Scene::TileSize);
                Scene::TileCfgA[i].HasCollision = (Uint8*)Memory::Calloc(1, Scene::TileSize);
            }
        }
        else if (Scene::TileSize != tileSize) {
            Scene::TileSize = tileSize;
            for (Uint32 i = 0; i < (tileCount << 1); i++) {
                Scene::TileCfgA[i].Collision = (Uint8*)Memory::Realloc(Scene::TileCfgA[i].Collision, Scene::TileSize);
                Scene::TileCfgA[i].HasCollision = (Uint8*)Memory::Realloc(Scene::TileCfgA[i].HasCollision, Scene::TileSize);
            }
        }

        for (Uint32 i = 0; i < tileCount; i++) {
            // Uint16 hasCollision = tileColReader->ReadUInt16();
            // Uint16 tileAngle    = tileColReader->ReadUInt16();
            // Scene::TileCfgA[i].IsCeiling = tileColReader->ReadByte();
            // Scene::TileCfgA[i].Config[0] = tileColReader->ReadByte();
            Scene::TileCfgB[i].IsCeiling = Scene::TileCfgA[i].IsCeiling = tileColReader->ReadByte();
            Scene::TileCfgB[i].Config[0] = Scene::TileCfgA[i].Config[0] = tileColReader->ReadByte();
            Scene::TileCfgA[i].Config[1] = false;
            bool hasCollision = tileColReader->ReadByte();
            for (int t = 0; t < tileSize; t++) {
                // Scene::TileCfgA[i].Collision[t] = tileColReader->ReadByte(); // collision
                // Scene::TileCfgA[i].HasCollision[t] = hasCollision; // (Scene::TileCfgA[i].Collision[t] != tileSize)
                Scene::TileCfgB[i].Collision[t] = Scene::TileCfgA[i].Collision[t] = tileColReader->ReadByte(); // collision
                Scene::TileCfgB[i].HasCollision[t] = Scene::TileCfgA[i].HasCollision[t] = hasCollision; // (Scene::TileCfgA[i].Collision[t] != tileSize)

                // Config[1] = IsThereEmptySpace
                Scene::TileCfgA[i].Config[1] |= (Scene::TileCfgA[i].Collision[t] == tileSize);
            }
            Scene::TileCfgB[i].Config[1] = Scene::TileCfgA[i].Config[1];
        }
    }
    tileColReader->Close();
}
PUBLIC STATIC void Scene::SaveTileCollisions() {
    if (DEBUG_lastTileColFilename == NULL) return;

    char yyyyyyyyyaaaaaa[256];
    sprintf(yyyyyyyyyaaaaaa, "Resources/%s", DEBUG_lastTileColFilename);

    Stream* tileColWriter;
    tileColWriter = FileStream::New(yyyyyyyyyaaaaaa, FileStream::WRITE_ACCESS);
    if (!tileColWriter) {
        Log::Print(Log::LOG_ERROR, "Could not open map file \"%s\"!", yyyyyyyyyaaaaaa);
        return;
    }

    tileColWriter->WriteUInt32(0x12345678);

    tileColWriter->WriteUInt32(Scene::TileCount);
    tileColWriter->WriteByte(Scene::TileSize);
    tileColWriter->WriteByte(0x00);
    tileColWriter->WriteByte(0x00);
    tileColWriter->WriteByte(0x00);
    tileColWriter->WriteUInt32(0x00000000);

    for (int i = 0; i < Scene::TileCount; i++) {
        tileColWriter->WriteByte(Scene::TileCfgA[i].IsCeiling);
        tileColWriter->WriteByte(Scene::TileCfgA[i].Config[0]);

        bool hasCollision = false;
        for (int t = 0; t < Scene::TileSize; t++) {
            hasCollision |= !!Scene::TileCfgA[i].HasCollision[t];
        }
        tileColWriter->WriteByte(hasCollision);

        for (int t = 0; t < Scene::TileSize; t++) {
            tileColWriter->WriteByte(Scene::TileCfgA[i].Collision[t]); // collision
        }
    }
    tileColWriter->Close();
}

PUBLIC STATIC void Scene::Update() {
    // Update objects
    for (Entity* ent = Scene::StaticObjectFirst, *next; ent; ent = next) {
        // Store the "next" so that when/if the current is removed,
        // it can still be used to point at the end of the loop.
        next = ent->NextEntity;

        // Execute whatever on object
        _UpdateObject(ent);
    }
    for (Entity* ent = Scene::DynamicObjectFirst, *next; ent; ent = next) {
        next = ent->NextEntity;

        _UpdateObject(ent);

        if (!ent->Active) {
            Scene::Remove(&Scene::DynamicObjectFirst, &Scene::DynamicObjectLast, &Scene::DynamicObjectCount, ent);
            ent->Dispose();
        }
    }

    #ifndef NO_LIBAV
    AudioManager::Lock();
    Uint8 audio_buffer[0x8000]; // <-- Should be larger than AudioManager::AudioQueueMaxSize
    int needed = 0x8000; // AudioManager::AudioQueueMaxSize;
    for (size_t i = 0, i_sz = Scene::MediaList.size(); i < i_sz; i++) {
        if (!Scene::MediaList[i])
            continue;

        MediaBag* media = Scene::MediaList[i]->AsMedia;
        int queued = (int)AudioManager::AudioQueueSize;
        if (queued < needed) {
            int ready_bytes = media->Player->GetAudioData(audio_buffer, needed - queued);
            if (ready_bytes > 0) {
                memcpy(AudioManager::AudioQueue + AudioManager::AudioQueueSize, audio_buffer, ready_bytes);
                AudioManager::AudioQueueSize += ready_bytes;
            }
        }
    }
    AudioManager::Unlock();
    #endif

    Scene::Frame++;
}

PUBLIC STATIC void Scene::Render() {
    Graphics::ResetViewport();

    for (int i = 0; i < 8; i++) {
        View* currentView = &Scene::Views[i];

        if (!currentView->Active)
            return;

        Scene::ViewCurrent = i;

        // NOTE: We should always be using the draw target.
        if (currentView->UseDrawTarget && currentView->DrawTarget) {
            float view_w = currentView->Width;
            float view_h = currentView->Height;
            Texture* tar = currentView->DrawTarget;
            if (tar->Width != view_w || tar->Height != view_h) {
                Graphics::DisposeTexture(tar);
                currentView->DrawTarget = Graphics::CreateTexture(SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, view_w, view_h);
            }

            Graphics::SetRenderTarget(currentView->DrawTarget);
            Graphics::Clear();
        }
        else if (!currentView->Visible) {
            continue;
        }

        // Adjust projection
        if (currentView->UsePerspective) {
            Graphics::UpdatePerspective(currentView->FOV, currentView->Width / currentView->Height, currentView->NearPlane, currentView->FarPlane);
            Matrix4x4::Rotate(currentView->ProjectionMatrix, currentView->ProjectionMatrix, currentView->RotateX, 1.0, 0.0, 0.0);
            Matrix4x4::Rotate(currentView->ProjectionMatrix, currentView->ProjectionMatrix, currentView->RotateY, 0.0, 1.0, 0.0);
            Matrix4x4::Rotate(currentView->ProjectionMatrix, currentView->ProjectionMatrix, currentView->RotateZ, 0.0, 0.0, 1.0);
            Matrix4x4::Translate(currentView->ProjectionMatrix, currentView->ProjectionMatrix, -currentView->X, -currentView->Y, -currentView->Z);
            Matrix4x4::Copy(currentView->BaseProjectionMatrix, currentView->ProjectionMatrix);
        }
        else {
            Graphics::UpdateOrtho(currentView->Width, currentView->Height);
            if (!currentView->UseDrawTarget)
                Graphics::UpdateOrthoFlipped(currentView->Width, currentView->Height);

            Matrix4x4::Rotate(currentView->ProjectionMatrix, currentView->ProjectionMatrix, currentView->RotateX, 1.0, 0.0, 0.0);
            Matrix4x4::Rotate(currentView->ProjectionMatrix, currentView->ProjectionMatrix, currentView->RotateY, 0.0, 1.0, 0.0);
            Matrix4x4::Rotate(currentView->ProjectionMatrix, currentView->ProjectionMatrix, currentView->RotateZ, 0.0, 0.0, 1.0);
            Matrix4x4::Translate(currentView->ProjectionMatrix, currentView->BaseProjectionMatrix, -std::floor(currentView->X), -std::floor(currentView->Y), -std::floor(currentView->Z));
        }
        Graphics::UpdateProjectionMatrix();

        // Graphics::SetBlendColor(0.5, 0.5, 0.5, 1.0);
        // Graphics::FillRectangle(currentView->X, currentView->Y, currentView->Width, currentView->Height);

        int layerSize = Layers.size() << PriorityPerLayer;
        int layerMask = (1 << PriorityPerLayer) - 1;
        int tileSize = Scene::TileSize;
        int tileSizeHalf = tileSize >> 1;
        int tileCellMaxWidth = 2 + (currentView->Width / tileSize);
        int tileCellMaxHeight = 2 + (currentView->Height / tileSize);

        // RenderEarly
        for (int l = 0; l < layerSize; l++) {
            if (DEV_NoObjectRender)
                break;

            size_t oSz = PriorityLists[l].size();
            for (size_t o = 0; o < oSz; o++) {
                if (PriorityLists[l][o] && PriorityLists[l][o]->Active)
                    PriorityLists[l][o]->RenderEarly();
            }
            // if (l == 2) {
            //     for (Entity* ent = Scene::StaticObjectFirst, *next; ent; ent = next) {
            //         next = ent->NextEntity;
            //         ent->RenderEarly();
            //     }
            //     for (Entity* ent = Scene::DynamicObjectFirst, *next; ent; ent = next) {
            //         next = ent->NextEntity;
            //         ent->RenderEarly();
            //     }
            // }
        }

        // Render
        for (int l = 0; l < layerSize; l++) {
            size_t oSz = PriorityLists[l].size();
            SceneLayer layer = Layers[l >> PriorityPerLayer];

            if (DEV_NoObjectRender)
                goto DEV_NoTilesCheck;

            for (size_t o = 0; o < oSz; o++) {
                if (PriorityLists[l][o] && PriorityLists[l][o]->Active)
                    PriorityLists[l][o]->Render(currentView->X, currentView->Y);
            }
            // if (l == 2) {
            //     for (Entity* ent = Scene::StaticObjectFirst, *next; ent; ent = next) {
            //         next = ent->NextEntity;
            //         ent->Render(currentView->X, currentView->Y);
            //     }
            //     for (Entity* ent = Scene::DynamicObjectFirst, *next; ent; ent = next) {
            //         next = ent->NextEntity;
            //         ent->Render(currentView->X, currentView->Y);
            //     }
            // }

            DEV_NoTilesCheck:
            if (DEV_NoTiles)
                continue;

            if (!Scene::TileSprite)
                continue;

            // Skip layer tile render if already rendered
            if ((l & layerMask) != 0)
                continue;

            // Draw Tiles
            if (layer.Visible) {
                int flipX, flipY;
                int TileBaseX, TileBaseY, baseX, baseY, tile, baseXOff, baseYOff;
                float tBX, tBY;

                Graphics::SetBlendColor(1.0, 1.0, 1.0, 1.0);
                // TileBaseX = (int)(currentView->X);
                // TileBaseY = (int)(currentView->Y);
                // if (layer.ScrollInfos) {
                //     baseXOff = (int)(currentView->X * (1.0f - layer.ScrollInfos[0].RelativeX / 256.f));
                // }
                // baseYOff = (currentView->Y * (1.0f - layer.RelativeY / 256.f));
                //
                // TileBaseX -= layer.OffsetX;
                // TileBaseY -= layer.OffsetY;
                // TileBaseX -= baseXOff;
                // TileBaseY -= baseYOff;

                tBX = currentView->X;
                tBY = currentView->Y;
                baseXOff = 0;
                if (layer.ScrollInfos) {
                    baseXOff = (int)std::ceil(currentView->X * (1.0f - layer.ScrollInfos[0].RelativeX / 256.f));
                }
                baseYOff = (int)std::ceil(currentView->Y * (1.0f - layer.RelativeY / 256.f));

                tBX -= layer.OffsetX;
                tBY -= layer.OffsetY;
                tBX -= baseXOff;
                tBY -= baseYOff;

                TileBaseX = (int)tBX;
                TileBaseY = (int)tBY;

                if (Graphics::SupportsBatching && false) { // && Scene::UseBatchedTiles
                    Graphics::Save();
                    Graphics::Translate(-TileBaseX, -TileBaseY, 0.0f);
                    GLRenderer::DrawTexturedShapeBuffer(Scene::TileSprite->Spritesheets[0], layer.BufferID, layer.VertexCount);
                    Graphics::Restore();
                    continue;
                }

                int tileCellStartX = (TileBaseX / tileSize);
                int tileCellStartY = (TileBaseY / tileSize);
                int tileCellEndX = tileCellStartX + tileCellMaxWidth;
                int ix = tileCellStartX,
                    iy = tileCellStartY,
                    sourceTileCellX, sourceTileCellY;

                for (int t = 0, fullSize = tileCellMaxWidth * tileCellMaxHeight; t < fullSize; t++) {
                    sourceTileCellX = ix;
                    sourceTileCellY = iy;

                    if (layer.Flags & SceneLayer::FLAGS_NO_REPEAT_X) {
                        if (sourceTileCellX < 0) goto LAYER_THICK_CONTINUE;
                        if (sourceTileCellX >= layer.Width) goto LAYER_THICK_CONTINUE;
                    }
                    if (layer.Flags & SceneLayer::FLAGS_NO_REPEAT_Y) {
                        if (sourceTileCellY < 0) goto LAYER_THICK_CONTINUE;
                        if (sourceTileCellY >= layer.Height) goto LAYER_THICK_CONTINUE;
                    }

                    // while (sourceTileCellX < 0) sourceTileCellX += layer.Width;
                    // while (sourceTileCellX >= layer.Width) sourceTileCellX -= layer.Width;
                    sourceTileCellX = ((sourceTileCellX % layer.Width) + layer.Width) % layer.Width;

                    // while (sourceTileCellY < 0) sourceTileCellY += layer.Height;
                    // while (sourceTileCellY >= layer.Height) sourceTileCellY -= layer.Height;
                    sourceTileCellY = ((sourceTileCellY % layer.Height) + layer.Height) % layer.Height;

                    baseX = (ix * tileSize) + tileSizeHalf;
                    baseY = (iy * tileSize) + tileSizeHalf;

                    baseX += baseXOff;
                    baseY += baseYOff;

                    tile = layer.Tiles[sourceTileCellX + sourceTileCellY * layer.Width];
                    flipX = (tile & TILE_FLIPX_MASK);
                    flipY = (tile & TILE_FLIPY_MASK);

                    tile &= TILE_IDENT_MASK;
                    if (tile != EmptyTile) {
                        Graphics::DrawSprite(TileSprite, 0, tile - FIRSTGID, baseX, baseY, flipX, flipY);
                    }

                    LAYER_THICK_CONTINUE:

                    ix++;
                    if (ix >= tileCellEndX) {
                        ix = tileCellStartX;
                        iy++;
                    }
                }

            }
        }

        // RenderLate
        for (int l = 0; l < layerSize; l++) {
            if (DEV_NoObjectRender)
                break;

            size_t oSz = PriorityLists[l].size();
            for (size_t o = 0; o < oSz; o++) {
                if (PriorityLists[l][o] && PriorityLists[l][o]->Active)
                    PriorityLists[l][o]->RenderLate();
            }
            // if (l == 2) {
            //     for (Entity* ent = Scene::StaticObjectFirst, *next; ent; ent = next) {
            //         next = ent->NextEntity;
            //         ent->RenderLate();
            //     }
            //     for (Entity* ent = Scene::DynamicObjectFirst, *next; ent; ent = next) {
            //         next = ent->NextEntity;
            //         ent->RenderLate();
            //     }
            // }
        }

        if (currentView->UseDrawTarget && currentView->DrawTarget) {
            Graphics::SetRenderTarget(NULL);
            if (currentView->Visible) {
                int w, h;
                SDL_GetWindowSize(Application::Window, &w, &h);

                Graphics::UpdateOrthoFlipped(w, h);
                Graphics::UpdateProjectionMatrix();

                Graphics::TextureBlend = false;
                Graphics::SetBlendMode(
                    BlendFactor_SRC_ALPHA, BlendFactor_INV_SRC_ALPHA,
                    BlendFactor_SRC_ALPHA, BlendFactor_INV_SRC_ALPHA);
                Graphics::DrawTexture(currentView->DrawTarget,
                    0.0, 0.0, currentView->Width, currentView->Height,
                    0.0, 0.0, w, h);
            }
        }
    }

    /*
    Graphics::DrawRectangle(0, 0, Application::WIDTH, Application::HEIGHT, BackgroundColor);

    int index, flipX, flipY, wheree, y;
    int TileBaseX, TileBaseY, baseX, baseY, tile, fullFlip;
    int layerSize = Layers.size() << PriorityPerLayer;
    int layerMask = (1 << PriorityPerLayer) - 1;

    for (int l = 0; l < layerSize; l++) {
        y = 0;

        for (int o = 0, oSz = PriorityLists[l].size(); o < oSz; o++) {
            if (PriorityLists[l][o] && PriorityLists[l][o]->Active)
                PriorityLists[l][o]->Render(Scene::CameraX, Scene::CameraY);
        }

        // Skip layer render if already rendered
        if ((l & layerMask) != 0)
            continue;

        SceneLayer layer = Layers[l >> PriorityPerLayer];
        // if (EditingMode > 0 && (l >> PriorityPerLayer) != EditingLayer)
            // continue;

        // Draw Tiles
        if (layer.Visible) {
            // Graphics::DoDeform = true;
            // memcpy(Graphics::Deform, layer.Deform, Application::HEIGHT);
            if (layer.ScrollInfoCount > 1) {
                // if (layer.UseDeltaCameraY)
                //     TileBaseY = (CameraDeltaY * layer.RelativeY + layer.ConstantY * Frame) >> 8;
                // else
                    TileBaseY = (Scene::CameraY * layer.RelativeY + layer.ConstantY * Frame) >> 8;

                TileBaseY -= layer.OffsetY;

                // int anID;
                int tileSize = 16;
                int tileSizeInBitShifts = 4;
                int tileSizeHalf = tileSize >> 1;
                int linesToDraw = Application::HEIGHT;
                if (layer.Flags & SceneLayer::FLAGS_NO_REPEAT_Y) {
                    linesToDraw = Math::Min(Application::HEIGHT, layer.Height * tileSize - TileBaseY);
                }
				// int tileCellStartX;
				int tileCellMaxWidth = 3 + (Application::WIDTH >> tileSizeInBitShifts);
				int ix = 0,
					iy = TileBaseY,
					fx = 0,
					fy = 0,
					tw = tileCellMaxWidth;
				for (int w = 0, fullSize = tileCellMaxWidth * linesToDraw; w < fullSize; w++) {
                    fx = ix;
					fy = iy;
					if (layer.Flags & SceneLayer::FLAGS_NO_REPEAT_X) {
						// if (fx < 0) {
						// 	// Increment
						// 	ix++;
						// 	if (ix >= tw) {
						// 		ix = tileCellStartX;
						// 		iy++;
						// 	}
						// 	continue;
						// }
						// if (fx >= layer.Width) {
						// 	// Increment
						// 	ix++;
						// 	if (ix >= tw) {
						// 		ix = tileCellStartX;
						// 		iy++;
						// 	}
						// 	continue;
						// }
					}
					if (layer.Flags & SceneLayer::FLAGS_NO_REPEAT_Y) {
						// if (fy < 0) {
						// 	// Increment
						// 	ix++;
						// 	if (ix >= tw) {
						// 		ix = tileCellStartX;
						// 		iy++;
						// 	}
						// 	continue;
						// }
						// if (fy >= layer.Height * tileSize) {
						// 	break;
						// }
					}

					while (fy < 0) fy += layer.Height << tileSizeInBitShifts;
					while (fy >= (layer.Height << tileSizeInBitShifts)) fy -= layer.Height << tileSizeInBitShifts;

                    index = layer.ScrollIndexes[fy];//.Index;

                    // if (layer.UseDeltaCameraX)
                    //     TileBaseX = (CameraDeltaX * layer.ScrollInfos[index].RelativeX + layer.ScrollInfos[index].ConstantX * Frame) >> 8;
                    // else
                        TileBaseX = (Scene::CameraX * layer.ScrollInfos[index].RelativeX + layer.ScrollInfos[index].ConstantX * Frame) >> 8;
                    TileBaseX -= layer.OffsetX;

                    fx += TileBaseX >> tileSizeInBitShifts;

					while (fx < 0) fx += layer.Width;
					while (fx >= layer.Width) fx -= layer.Width;

					baseX = (ix << tileSizeInBitShifts) - (TileBaseX & (tileSize - 1));
					baseY = iy - TileBaseY; // + layer.TileOffsetY[fx];

					tile = layer.Tiles[fx + (fy >> tileSizeInBitShifts) * layer.Width];
					fullFlip = (tile >> 10) & 3;
                    flipY = ((fullFlip >> 1) & 1);
					tile = tile & 0x3FF;

                    wheree = fy & (tileSize - 1);
                    if (flipY)
                        wheree ^= (tileSize - 1);

					if (tile != EmptyTile) {
						// anID = Data->isAnims[tile] & 0xFF;
						// if (anID != 0xFF)
						// 	Graphics::DrawSprite(AnimTileSprite, Data->isAnims[tile] >> 8, Data->animatedTileFrames[anID], baseX + 8, baseY + 8, 0, fullFlip);
						// else
							Graphics::DrawSpritePart(TileSprite, 0, tile, 0, wheree, tileSize, 1, baseX + tileSizeHalf, baseY + tileSizeHalf, fullFlip & 1, false);
					}

					ix++;
					if (ix >= tw) {
						ix = 0;
						iy++;
					}
				}
            }
            else {
                // if (layer.UseDeltaCameraX)
                //     TileBaseX = (CameraDeltaX * layer.ScrollInfos[0].RelativeX + layer.ScrollInfos[0].ConstantX * Frame) >> 8;
                // else
                    TileBaseX = (Scene::CameraX * layer.ScrollInfos[0].RelativeX + layer.ScrollInfos[0].ConstantX * Frame) >> 8;

                // if (layer.UseDeltaCameraY)
                //     TileBaseY = (CameraDeltaY * layer.RelativeY + layer.ConstantY * Frame) >> 8;
                // else
                    TileBaseY = (Scene::CameraY * layer.RelativeY + layer.ConstantY * Frame) >> 8;

                TileBaseX -= layer.OffsetX;
                TileBaseY -= layer.OffsetY;

                // if (EditingMode > 0) {
                //     TileBaseX = Scene::CameraX;
                //     TileBaseY = Scene::CameraY;
                // }

                // EndTileBaseX = 2 + ((TileBaseX + Application::WIDTH) >> 4);
                // EndTileBaseY = 2 + ((TileBaseY + Application::HEIGHT) >> 4);

                //int lWid = layer.Width;

                // int j;
				// int bufVal;
				// int anID;
                int tileSize = 16;
                int tileSizeHalf = tileSize >> 1;
				int tileCellStartX = (TileBaseX >> 4);
				int tileCellMaxWidth = 2 + (Application::WIDTH >> 4);
				int tileCellMaxHeight = 2 + (Application::HEIGHT >> 4);
				int ix = tileCellStartX,
					iy = (TileBaseY >> 4),
					fx = 0,
					fy = 0,
					tw = tileCellStartX + tileCellMaxWidth;
				for (int w = 0, fullSize = tileCellMaxWidth * tileCellMaxHeight; w < fullSize; w++) {
					fx = ix;
					fy = iy;
					if (layer.Flags & SceneLayer::FLAGS_NO_REPEAT_X) {
					    if (fx < 0) goto LAYER_THICK_CONTINUE;
					    if (fx >= layer.Width) goto LAYER_THICK_CONTINUE;
					}
					if (layer.Flags & SceneLayer::FLAGS_NO_REPEAT_Y) {
					    if (fy < 0) goto LAYER_THICK_CONTINUE;
					    if (fy >= layer.Height) goto LAYER_THICK_CONTINUE;
					}

					while (fx < 0) fx += layer.Width;
					while (fx >= layer.Width) fx -= layer.Width;

					while (fy < 0) fy += layer.Height;
					while (fy >= layer.Height) fy -= layer.Height;

					baseX = (ix << 4) - TileBaseX;
					baseY = (iy << 4) - TileBaseY; // + layer.TileOffsetY[fx];

					tile = layer.Tiles[fx + fy * layer.Width];
					flipX = (tile >> 10) & 1;
                    flipY = (tile >> 11) & 1;
					tile = tile & 0x3FF;

                    // DoEditModeShit1(baseX, baseY, fx, fy, layer.Tiles[fx + fy * layer.Width], 16);

					if (tile != EmptyTile) {
						// anID = Data->isAnims[tile] & 0xFF;
						// if (anID != 0xFF)
						// 	Graphics::DrawSprite(AnimTileSprite, Data->isAnims[tile] >> 8, Data->animatedTileFrames[anID], baseX + 8, baseY + 8, 0, fullFlip);
						// else
							Graphics::DrawSprite(TileSprite, 0, tile, baseX + tileSizeHalf, baseY + tileSizeHalf, flipX, flipY);
					}

                    LAYER_THICK_CONTINUE:

					ix++;
					if (ix >= tw) {
						ix = tileCellStartX;
						iy++;
					}
				}
            }
        }
    }
    //*/
}

PUBLIC STATIC void Scene::AfterScene() {
    BytecodeObjectManager::ResetStack();
    BytecodeObjectManager::RequestGarbageCollection();
    if (Scene::NextScene[0]) {
        Scene::LoadScene(Scene::NextScene);
        Scene::Restart();

        Scene::NextScene[0] = '\0';
        Scene::DoRestart = false;
    }
    if (Scene::DoRestart) {
        Scene::Restart();
        Scene::DoRestart = false;
    }
}

PUBLIC STATIC void Scene::Dispose() {
    delete Scene::Views[0].ProjectionMatrix;

    // Dispose of all SCOPE_SCENE sprites and images
    for (size_t i = 0, i_sz = Scene::SpriteList.size(); i < i_sz; i++) {
        if (!Scene::SpriteList[i]) continue;

        Scene::SpriteList[i]->AsSprite->Dispose();
        delete Scene::SpriteList[i]->AsSprite;
    }
    for (size_t i = 0, i_sz = Scene::ImageList.size(); i < i_sz; i++) {
        if (!Scene::ImageList[i]) continue;

        Scene::ImageList[i]->AsImage->Dispose();
        delete Scene::ImageList[i]->AsImage;
    }
    Scene::SpriteList.clear();
    Scene::ImageList.clear();

    AudioManager::Lock();
    // Make sure no audio is playing (really we don't want any audio to be playing, otherwise we'll get a EXC_BAD_ACCESS)
    AudioManager::ClearMusic();
    AudioManager::AudioPauseAll();

    // Dispose of all SCOPE_SCENE sounds and music
    for (size_t i = 0, i_sz = Scene::SoundList.size(); i < i_sz; i++) {
        Scene::SoundList[i]->AsSound->Dispose();
        delete Scene::SoundList[i]->AsSound;
    }
    for (size_t i = 0, i_sz = Scene::MusicList.size(); i < i_sz; i++) {
        Scene::MusicList[i]->AsMusic->Dispose();
        delete Scene::MusicList[i]->AsMusic;
    }
    for (size_t i = 0, i_sz = Scene::MediaList.size(); i < i_sz; i++) {
        if (!Scene::MediaList[i]) continue;

        #ifndef NO_LIBAV
        Scene::MediaList[i]->AsMedia->Player->Close();
        Scene::MediaList[i]->AsMedia->Source->Close();
        #endif
        delete Scene::MediaList[i]->AsMedia;
    }
    Scene::SoundList.clear();
    Scene::MusicList.clear();
    Scene::MediaList.clear();

    AudioManager::Unlock();

    // Dispose and clear Static objects
    for (Entity* ent = Scene::StaticObjectFirst, *next; ent; ent = next) {
        next = ent->NextEntity;
        ent->Dispose();
    }
    Scene::Clear(&Scene::StaticObjectFirst, &Scene::StaticObjectLast, &Scene::StaticObjectCount);

    // Dispose and clear Dynamic objects
    for (Entity* ent = Scene::DynamicObjectFirst, *next; ent; ent = next) {
        next = ent->NextEntity;
        ent->Dispose();
    }
    Scene::Clear(&Scene::DynamicObjectFirst, &Scene::DynamicObjectLast, &Scene::DynamicObjectCount);

    // Free Priority Lists
    Memory::Free(Scene::PriorityLists);
    Scene::PriorityLists = NULL;

    for (size_t i = 0; i < Scene::Layers.size(); i++) {
        Scene::Layers[i].Dispose();
    }
    Scene::Layers.clear();

    if (Scene::TileSprite) {
        Scene::TileSprite->Dispose();
        delete Scene::TileSprite;
    }
    Scene::TileSprite = NULL;

    if (Scene::ObjectLists)
        delete Scene::ObjectLists;
    Scene::ObjectLists = NULL;

    if (Scene::ObjectRegistries)
        delete Scene::ObjectRegistries;
    Scene::ObjectRegistries = NULL;

    if (Scene::ExtraPalettes)
        Memory::Free(Scene::ExtraPalettes);
    Scene::ExtraPalettes = NULL;

    if (Scene::TileCfgA) {
        for (int i = 0; i < (Scene::TileCount << 1); i++) {
            Memory::Free(Scene::TileCfgA[i].Collision);
            Memory::Free(Scene::TileCfgA[i].HasCollision);
        }
        Memory::Free(Scene::TileCfgA);
    }
    Scene::TileCfgA = NULL;

    BytecodeObjectManager::Dispose();
    SourceFileMap::Dispose();
}

PUBLIC STATIC void Scene::Exit() {

}

PUBLIC STATIC int  Scene::CollisionAt(int x, int y, int collisionField, int collideSide, int* angle) {
    int temp;
    int checkX;
    int probeXOG = x;
    int probeYOG = y;
    int tileX, tileY, tileID, tileSize, tileAngle;
    int flipX, flipY, collisionA, collisionB, collision;
    int height, hasCollision, isCeiling;

    bool check, checkTop, checkBottom;
    TileConfig* tileCfg;

    bool wallAsFloorFlag = collideSide & 0x10;
    collideSide &= 0xF;

    int configIndex = 0, configIndexCopy, configH, configV;
    configH = configV = false;
    switch (collideSide) {
        case CollideSide::TOP:
            configIndex = 0;
            configV = true;
            break;
        case CollideSide::LEFT:
            configIndex = 1;
            configH = true;
            break;
        case CollideSide::RIGHT:
            configIndex = 2;
            configH = true;
            break;
        case CollideSide::BOTTOM:
            configIndex = 3;
            configV = true;
            break;
    }

    if (collisionField < 0)
        return -1;
    if (collisionField > 1)
        return -1;
    if (!Scene::TileCfgA && collisionField == 0)
        return -1;
    if (!Scene::TileCfgB && collisionField == 1)
        return -1;

    tileSize = 16;

    for (size_t l = 0, lSz = Layers.size(); l < lSz; l++) {
        SceneLayer layer = Layers[l];
        if (!(layer.Flags & SceneLayer::FLAGS_COLLIDEABLE))
            continue;

        x = probeXOG;
        y = probeYOG;

        x -= layer.OffsetX;
        y -= layer.OffsetY;

        // Check Layer Width
        temp = layer.Width << 4;
        if ((layer.Flags & SceneLayer::FLAGS_NO_REPEAT_X) && (x < 0 || x >= temp))
            continue;
        x = ((x % temp) + temp) % temp;

        // Check Layer Height
        temp = layer.Height << 4;
        if ((layer.Flags & SceneLayer::FLAGS_NO_REPEAT_Y) && (y < 0 || y >= temp))
            continue;
        y = ((y % temp) + temp) % temp;

        tileX = x >> 4;
        tileY = y >> 4;

        tileID = layer.Tiles[tileX + tileY * layer.Width];
        if ((tileID & TILE_IDENT_MASK) != EmptyTile) {
            flipX      = !!(tileID & TILE_FLIPX_MASK);
            flipY      = !!(tileID & TILE_FLIPY_MASK);
            collisionA = (tileID & TILE_COLLA_MASK) >> 28;
            collisionB = (tileID & TILE_COLLB_MASK) >> 26;
            // collisionC = (tileID & TILE_COLLC_MASK) >> 24;
            collision  = collisionField ? collisionB : collisionA;
            tileID = tileID & TILE_IDENT_MASK;

            // Alter check X
            checkX = x & 0xF;
            if (flipX)
                checkX ^= 0xF;

            // Check tile config
            tileCfg = collisionField ? &Scene::TileCfgB[tileID] : &Scene::TileCfgA[tileID];
            height       = tileCfg->Collision[checkX];
            hasCollision = tileCfg->HasCollision[checkX];
            isCeiling    = tileCfg->IsCeiling;
            if (!hasCollision)
                continue;

            // Check if we can collide with the tile side
            check = false;
            check |= (collision & 1) && (collideSide & CollideSide::TOP);
            check |= wallAsFloorFlag && ((collision & 1) && (collideSide & (CollideSide::LEFT | CollideSide::RIGHT)));
            check |= (collision & 2) && (collideSide & CollideSide::BOTTOM_SIDES);
            if (!check)
                continue;

            // Check Y
            tileY = tileY << 4;
            checkTop    =  (isCeiling ^ flipY) && (y >= tileY && y < tileY + tileSize - height);
            checkBottom = !(isCeiling ^ flipY) && (y >= tileY + height && y < tileY + tileSize);
            if (!checkTop && !checkBottom)
                continue;


            // Determine correct angle to use
            configIndexCopy = configIndex;
            if (flipX && configH)
                configIndexCopy ^= 0x3;
            if (flipY && configV)
                configIndexCopy ^= 0x3;

            // Return angle
            tileAngle = tileCfg->Config[configIndexCopy];
            if (tileAngle != 0xFF) {
                if (flipX) {
                    tileAngle ^= 0xFF; tileAngle++;
                }
                if (flipY) {
                    tileAngle ^= 0x7F; tileAngle++;
                }
            }
            return tileAngle & 0xFF;
        }
    }

    return -1;
}
