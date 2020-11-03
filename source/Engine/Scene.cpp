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
    static int                   ShowTileCollisionFlag;

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
    static vector<GLShader*>     ShaderList;
    static vector<ResourceType*> ModelList;
    static vector<ResourceType*> MediaList;

    static int                   Frame;
    static bool                  Paused;
    static int                   MainLayer;

    static View                  Views[8];
    static int                   ViewCurrent;
    static float                 PreGameRenderViewRenderTimes[8];
    static float                 GameRenderEarlyViewRenderTimes[8];
    static float                 GameRenderViewRenderTimes[8];
    static float                 GameRenderLateViewRenderTimes[8];
    static float                 PostGameRenderViewRenderTimes[8];

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
#include <Engine/ResourceTypes/SceneFormats/TiledMapReader.h>
#include <Engine/TextFormats/XML/XMLParser.h>
#include <Engine/Types/EntityTypes.h>
#include <Engine/Includes/HashMap.h>
#include <Engine/Types/ObjectList.h>

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
int                   Scene::PriorityPerLayer = 16;
vector<Entity*>*      Scene::PriorityLists = NULL;

// Rendering variables
Uint32                Scene::BackgroundColor = 0x000000;
int                   Scene::ShowTileCollisionFlag = 0;

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
float                 Scene::PreGameRenderViewRenderTimes[8];
float                 Scene::GameRenderEarlyViewRenderTimes[8];
float                 Scene::GameRenderViewRenderTimes[8];
float                 Scene::GameRenderLateViewRenderTimes[8];
float                 Scene::PostGameRenderViewRenderTimes[8];

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

Entity*               StaticObject = NULL;
bool                  DEV_NoTiles = false;
bool                  DEV_NoObjectRender = false;
const char*           DEBUG_lastTileColFilename = NULL;

int SCOPE_SCENE = 0;
int SCOPE_GAME = 1;
int TileViewRenderFlag = 0x01;
int TileBatchMaxSize = 8;

struct TileBatch {
    Uint32 BufferID;
    Uint32 VertexCount;
    Uint32 Width;
    Uint32 Height;
};

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
void _ObjectList_ResetPerf(Uint32, ObjectList* list) {
    // list->AverageUpdateTime = 0.0;
    list->AverageUpdateItemCount = 0.0;
    list->AverageRenderItemCount = 0.0;
}
void _UpdateObjectEarly(Entity* ent) {
    if (Scene::Paused && ent->Pauseable)
        return;
    if (!ent->Active)
        return;
    if (!ent->OnScreen)
        return;

    ent->UpdateEarly();
}
void _UpdateObjectLate(Entity* ent) {
    if (Scene::Paused && ent->Pauseable)
        return;
    if (!ent->Active)
        return;
    if (!ent->OnScreen)
        return;

    ent->UpdateLate();
}
void _UpdateObject(Entity* ent) {
    if (Scene::Paused && ent->Pauseable)
        return;

    if (!ent->Active)
        return;

    if ((ent->OnScreenHitboxW == 0.0f || (
        ent->X + ent->OnScreenHitboxW * 0.5f >= Scene::Views[0].X &&
        ent->X - ent->OnScreenHitboxW * 0.5f <  Scene::Views[0].X + Scene::Views[0].Width)) &&
        (ent->OnScreenHitboxH == 0.0f || (
        ent->Y + ent->OnScreenHitboxH * 0.5f >= Scene::Views[0].Y &&
        ent->Y - ent->OnScreenHitboxH * 0.5f <  Scene::Views[0].Y + Scene::Views[0].Height))) {
        double elapsed = Clock::GetTicks();

        ent->OnScreen = true;

        ent->Update();

        elapsed = Clock::GetTicks() - elapsed;

        if (ent->List) {
            ObjectList* list = ent->List;
            double count = list->AverageUpdateItemCount;
            if (count < 60.0 * 60.0) {
                count += 1.0;
                if (count == 1.0)
                    list->AverageUpdateTime = elapsed;
                else
                    list->AverageUpdateTime =
                        list->AverageUpdateTime + (elapsed - list->AverageUpdateTime) / count;
                list->AverageUpdateItemCount = count;
            }
        }
    }
    else {
        ent->OnScreen = false;
    }

    if (!Scene::PriorityLists)
        return;

    int oldPriority = ent->PriorityOld;
    int maxPriority = Scene::PriorityPerLayer - 1;
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

// Scene Lifecycle
PUBLIC STATIC void Scene::Init() {
    Scene::NextScene[0] = '\0';
    Scene::CurrentScene[0] = '\0';

    SourceFileMap::CheckForUpdate();

    Application::GameStart = true;

    BytecodeObjectManager::Init();
    BytecodeObjectManager::ResetStack();
    BytecodeObjectManager::LinkStandardLibrary();

    Application::Settings->GetBool("dev", "notiles", &DEV_NoTiles);
    Application::Settings->GetBool("dev", "noobjectrender", &DEV_NoObjectRender);
    Application::Settings->GetInteger("dev", "viewCollision", &ShowTileCollisionFlag);

    Graphics::SetTextureInterpolation(false);

    Scene::ViewCurrent = 0;
    for (int i = 0; i < 8; i++) {
        Scene::Views[i].Active = false;
        Scene::Views[i].Software = false;
        Scene::Views[i].Width = 640;
        Scene::Views[i].Height = 480;
        Scene::Views[i].FOV = 45.0f;
        Scene::Views[i].UsePerspective = false;
        Scene::Views[i].DrawTarget = Graphics::CreateTexture(SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, Scene::Views[i].Width, Scene::Views[i].Height);
        Scene::Views[i].UseDrawTarget = true;
        Scene::Views[i].ProjectionMatrix = Matrix4x4::Create();
        Scene::Views[i].BaseProjectionMatrix = Matrix4x4::Create();
    }
    Scene::Views[0].Active = true;
}

PUBLIC STATIC void Scene::Update() {
    if (Scene::ObjectLists)
        Scene::ObjectLists->ForAll(_ObjectList_ResetPerf);

    // Early Update
    for (Entity* ent = Scene::StaticObjectFirst, *next; ent; ent = next) {
        next = ent->NextEntity;
        _UpdateObjectEarly(ent);
    }
    for (Entity* ent = Scene::DynamicObjectFirst, *next; ent; ent = next) {
        next = ent->NextEntity;
        _UpdateObjectEarly(ent);
    }

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
    }

    // Late Update
    for (Entity* ent = Scene::StaticObjectFirst, *next; ent; ent = next) {
        next = ent->NextEntity;
        _UpdateObjectLate(ent);
    }
    for (Entity* ent = Scene::DynamicObjectFirst, *next; ent; ent = next) {
        next = ent->NextEntity;
        _UpdateObjectLate(ent);

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

    if (!Scene::Paused)
        Scene::Frame++;
}
PUBLIC STATIC void Scene::Render() {
    if (!Scene::PriorityLists)
        return;

    Graphics::ResetViewport();

    float cx, cy, cz;

    // DEV_NoTiles = true;
    // DEV_NoObjectRender = true;

    int win_w, win_h;
    SDL_GetWindowSize(Application::Window, &win_w, &win_h);

    int view_count = 8;
    for (int i = 0; i < view_count; i++) {
        View* currentView = &Scene::Views[i];

        if (!currentView->Active)
            continue;

        Scene::PreGameRenderViewRenderTimes[i] = Clock::GetTicks();

        cx = std::floor(currentView->X);
        cy = std::floor(currentView->Y);
        cz = std::floor(currentView->Z);

        Scene::ViewCurrent = i;

        int viewRenderFlag = 1 << i;

        // NOTE: We should always be using the draw target.
        if (currentView->UseDrawTarget && currentView->DrawTarget) {
            float view_w = currentView->Width;
            float view_h = currentView->Height;
            Texture* tar = currentView->DrawTarget;
            if (tar->Width != view_w || tar->Height != view_h) {
                Graphics::DisposeTexture(tar);
                Graphics::SetTextureInterpolation(false);
                currentView->DrawTarget = Graphics::CreateTexture(SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, view_w, view_h);
            }

            Graphics::SetRenderTarget(currentView->DrawTarget);

            if (currentView->Software) {
                Graphics::SoftwareStart();
            }
            else {
                Graphics::Clear();
            }
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
            Matrix4x4::Translate(currentView->ProjectionMatrix, currentView->BaseProjectionMatrix, -cx, -cy, -cz);
        }
        Graphics::UpdateProjectionMatrix();

        Scene::PreGameRenderViewRenderTimes[i] = Clock::GetTicks() - Scene::PreGameRenderViewRenderTimes[i];

        // Graphics::SetBlendColor(0.5, 0.5, 0.5, 1.0);
        // Graphics::FillRectangle(currentView->X, currentView->Y, currentView->Width, currentView->Height);


        // RenderEarly
        Scene::GameRenderEarlyViewRenderTimes[i] = Clock::GetTicks();
        for (int l = 0; l < Scene::PriorityPerLayer; l++) {
            if (DEV_NoObjectRender)
                break;

            size_t oSz = PriorityLists[l].size();
            for (size_t o = 0; o < oSz; o++) {
                if (PriorityLists[l][o] && PriorityLists[l][o]->Active)
                    PriorityLists[l][o]->RenderEarly();
            }
        }
        Scene::GameRenderEarlyViewRenderTimes[i] = Clock::GetTicks() - Scene::GameRenderEarlyViewRenderTimes[i];

        // Render
        register float _vx = currentView->X;
        register float _vy = currentView->Y;
        register float _vw = currentView->Width;
        register float _vh = currentView->Height;
        Scene::GameRenderViewRenderTimes[i] = Clock::GetTicks();
        for (int l = 0; l < Scene::PriorityPerLayer; l++) {
            size_t oSz = PriorityLists[l].size();

            if (DEV_NoObjectRender)
                goto DEV_NoTilesCheck;

            double elapsed;
            register float hbW;
            register float hbH;
            register float _x1;
            register float _x2;
            register float _y1;
            register float _y2;
            register float _ox;
            register float _oy;
            for (size_t o = 0; o < oSz; o++) {
                if (PriorityLists[l][o] && PriorityLists[l][o]->Active) {
                    Entity* ent = PriorityLists[l][o];

                    if (ent->RenderRegionW == 0.0f || ent->RenderRegionH == 0.0f)
                        goto DoCheckRender;

                    hbW = ent->RenderRegionW * 0.5f;
                    hbH = ent->RenderRegionH * 0.5f;
                    _ox = ent->X - _vx;
                    _oy = ent->Y - _vy;
                    if ((_ox + hbW) < 0.0f || (_ox - hbW) >= _vw ||
                        (_oy + hbH) < 0.0f || (_oy - hbH) >= _vh)
                        continue;

                    DoCheckRender:
                    if (!(ent->ViewRenderFlag & viewRenderFlag))
                        continue;

                    elapsed = Clock::GetTicks();

                    ent->Render(currentView->X, currentView->Y);

                    elapsed = Clock::GetTicks() - elapsed;

                    if (ent->List) {
                        ObjectList* list = ent->List;
                        double count = list->AverageRenderItemCount;
                        if (count < 60.0 * 60.0) {
                            count += 1.0;
                            if (count == 1.0)
                                list->AverageRenderTime = elapsed;
                            else
                                list->AverageRenderTime =
                                    list->AverageRenderTime + (elapsed - list->AverageRenderTime) / count;
                            list->AverageRenderItemCount = count;
                        }
                    }
                }
            }

            DEV_NoTilesCheck:
            if (DEV_NoTiles)
                continue;

            if (!Scene::TileSprite)
                continue;

            if (!(TileViewRenderFlag & viewRenderFlag))
                continue;

            bool texBlend = Graphics::TextureBlend;

            Graphics::TextureBlend = false;
            for (size_t li = 0; li < Layers.size(); li++) {
                SceneLayer layer = Layers[li];
                // Skip layer tile render if already rendered
                if (layer.DrawGroup != l)
                    continue;

                // Draw Tiles
                if (layer.Visible) {
                    Graphics::Save();
                    Graphics::Translate(cx, cy, cz);

                    int tileSize = Scene::TileSize;
                    int tileSizeHalf = tileSize >> 1;
                    int tileCellMaxWidth = 3 + (currentView->Width / tileSize);
                    int tileCellMaxHeight = 2 + (currentView->Height / tileSize);

                    int flipX, flipY, col;
                    int TileBaseX, TileBaseY, baseX, baseY, tile, tileOrig, baseXOff, baseYOff;
                    float tBX, tBY;

                    TileConfig* baseTileCfg = Scene::ShowTileCollisionFlag == 2 ? Scene::TileCfgB : Scene::TileCfgA;

                    int layerWidth = layer.Width / tileSize;
                    int layerHeight = layer.Height / tileSize;

                    Graphics::SetBlendColor(1.0, 1.0, 1.0, 1.0);

                    if (false && (!layer.ScrollInfosSplitIndexes || layer.ScrollInfosSplitIndexesCount == 0 || layer.RelativeY == 0x100)) {
                        baseXOff = (int)std::floor(currentView->X);
                        baseYOff = (int)std::floor(currentView->Y);
                        baseXOff += layer.OffsetX;
                        baseYOff += layer.OffsetY;

                        TileBaseX = baseXOff;
                        TileBaseY = baseYOff;

                        int xxx = cx;
                        int yyy = cy;
                        // int xxx = baseXOff % layerWidth;
                        // int yyy = baseYOff % layerHeight;
                        // if (xxx < 0) xxx += layerWidth;
                        // if (yyy < 0) yyy += layerHeight;

                        // xxx += ;
                        // yyy += ;

                        Graphics::Translate(-xxx, -yyy, 0.0f);
                        int batch_start_x = (int)(baseXOff / tileSize) / TileBatchMaxSize - 1;
                        int batch_start_y = (int)(baseYOff / tileSize) / TileBatchMaxSize - 1;
                        int batch_end_x = ((int)(baseXOff + currentView->Width) / tileSize) / TileBatchMaxSize + 2;
                        int batch_end_y = ((int)(baseYOff + currentView->Height) / tileSize) / TileBatchMaxSize + 2;

                        int batch_count_x = (layer.Width + TileBatchMaxSize - 1) / TileBatchMaxSize;
                        int batch_count_y = (layer.Height + TileBatchMaxSize - 1) / TileBatchMaxSize;
                        if (batch_start_x < 0)
                            batch_start_x = 0;
                        if (batch_start_y < 0)
                            batch_start_y = 0;
                        if (batch_end_x > batch_count_x)
                            batch_end_x = batch_count_x;
                        if (batch_end_y > batch_count_y)
                            batch_end_y = batch_count_y;

                        for (int x = batch_start_x; x < batch_end_x; x++) {
                            for (int y = batch_start_y; y < batch_end_y; y++) {
                                TileBatch* tileBatch = &((TileBatch*)layer.TileBatches)[x + y * batch_count_x];
                                // Graphics::Save();
                                // Graphics::Translate(-xxx + x * 1.0f, -yyy + y * 1.0, 0.0f);
                                    GLRenderer::DrawTexturedShapeBuffer(Scene::TileSprite->Spritesheets[0], tileBatch->BufferID, tileBatch->VertexCount);
                                // Graphics::Restore();
                            }
                        }

                        // if (Graphics::SupportsBatching && Application::Platform == Platforms::Android && false) { // && Scene::UseBatchedTiles
                        //     int xxx = baseXOff % layerWidth;
                        //     int yyy = baseYOff % layerHeight;
                        //     if (xxx < 0) xxx += layerWidth;
                        //     if (yyy < 0) yyy += layerHeight;
                        //
                        //     Graphics::Save();
                        //     Graphics::Translate(-xxx, -yyy, 0.0f);
                        //         GLRenderer::DrawTexturedShapeBuffer(Scene::TileSprite->Spritesheets[0], layer.BufferID, layer.VertexCount);
                        //         if (!(layer.Flags & SceneLayer::FLAGS_NO_REPEAT_X)) {
                        //             Graphics::Translate(layerWidth, 0.0f, 0.0f);
                        //             GLRenderer::DrawTexturedShapeBuffer(Scene::TileSprite->Spritesheets[0], layer.BufferID, layer.VertexCount);
                        //         }
                        //         if (layerWidth < currentView->Width) {
                        //             Graphics::Translate(layerWidth, 0.0f, 0.0f);
                        //             GLRenderer::DrawTexturedShapeBuffer(Scene::TileSprite->Spritesheets[0], layer.BufferID, layer.VertexCount);
                        //         }
                        //     Graphics::Restore();
                        //
                        //     Graphics::Restore();
                        //     continue;
                        // }
                    }
                    else {
                        if (layer.ScrollInfosSplitIndexes && layer.ScrollInfosSplitIndexesCount > 0) {
                            int height, heightHalf, tileY, index;
                            int ix, sourceTileCellX, sourceTileCellY;

                            // baseYOff = (int)std::floor(
                            //     (currentView->Y + layer.OffsetY) * layer.RelativeY / 256.f +
                            //     Scene::Frame * layer.ConstantY / 256.f);
                            baseYOff = ((((int)currentView->Y + layer.OffsetY) * layer.RelativeY) + Scene::Frame * layer.ConstantY) >> 8;

                            if (Graphics::SupportsBatching && Application::Platform == Platforms::Android && false) { // && Scene::UseBatchedTiles
                                baseXOff = (int)std::floor(
                                    (currentView->X + layer.OffsetX) * layer.ScrollInfos[0].RelativeX / 256.f +
                                    Scene::Frame * layer.ScrollInfos[0].ConstantX / 256.f);

                                int xxx = baseXOff;
                                int yyy = baseYOff;
                                // if (!(layer.Flags & SceneLayer::FLAGS_NO_REPEAT_X)) {
                                //     xxx %= layerWidth * tileSize;
                                //     if (xxx < 0) xxx += layerWidth * tileSize;
                                // }
                                // if (!(layer.Flags & SceneLayer::FLAGS_NO_REPEAT_Y)) {
                                //     yyy %= layerHeight * tileSize;
                                //     if (yyy < 0) yyy += layerHeight * tileSize;
                                // }

                                // Graphics::Save();
                                // Graphics::Translate(-xxx, -yyy, 0.0f);
                                    // GLRenderer::DrawTexturedShapeBuffer(Scene::TileSprite->Spritesheets[0], layer.BufferID, layer.VertexCount);
                                    // if (!(layer.Flags & SceneLayer::FLAGS_NO_REPEAT_X)) {
                                    //     Graphics::Translate(layerWidth, 0.0f, 0.0f);
                                    //     GLRenderer::DrawTexturedShapeBuffer(Scene::TileSprite->Spritesheets[0], layer.BufferID, layer.VertexCount);
                                    // }
                                    // if (layerWidth < currentView->Width) {
                                    //     Graphics::Translate(layerWidth, 0.0f, 0.0f);
                                    //     GLRenderer::DrawTexturedShapeBuffer(Scene::TileSprite->Spritesheets[0], layer.BufferID, layer.VertexCount);
                                    // }
                                // Graphics::Restore();

                                Graphics::Restore();
                                continue;
                            }

                            TileBaseY = 0;

                            for (int split = 0, spl; split < 4096; split++) {
                                spl = split;
                                while (spl >= layer.ScrollInfosSplitIndexesCount) spl -= layer.ScrollInfosSplitIndexesCount;

                                height = (layer.ScrollInfosSplitIndexes[spl] >> 8) & 0xFF;

                                sourceTileCellY = (TileBaseY >> 4);
                                baseY = (sourceTileCellY << 4) + 8;
                                baseY -= baseYOff;

                                if (baseY - 8 + height < -tileSize)
                                    goto SKIP_TILE_ROW_DRAW;
                                if (baseY - 8 >= currentView->Height + tileSize)
                                    break;

                                index = layer.ScrollInfosSplitIndexes[spl] & 0xFF;
                                // baseXOff = (int)std::floor(
                                //     (currentView->X + layer.OffsetX) * layer.ScrollInfos[index].RelativeX / 256.f +
                                //     Scene::Frame * layer.ScrollInfos[index].ConstantX / 256.f);
                                baseXOff = ((((int)currentView->X + layer.OffsetX) * layer.ScrollInfos[index].RelativeX) + Scene::Frame * layer.ScrollInfos[index].ConstantX) >> 8;
                                TileBaseX = baseXOff;

                                // Loop or cut off sourceTileCellY
                                if (layer.Flags & SceneLayer::FLAGS_NO_REPEAT_Y) {
                                    if (sourceTileCellY < 0) goto SKIP_TILE_ROW_DRAW;
                                    if (sourceTileCellY >= layer.Height) goto SKIP_TILE_ROW_DRAW;
                                }
                                sourceTileCellY = sourceTileCellY & layer.HeightMask;

                                // Draw row of tiles
                                ix = (TileBaseX >> 4);
                                baseX = tileSizeHalf;
                                baseX -= baseXOff & 15; // baseX -= baseXOff % 16;

                                // To get the leftmost tile, ix--, and start t = -1
                                baseX -= 16;
                                ix--;

                                // sourceTileCellX = ((sourceTileCellX % layer.Width) + layer.Width) % layer.Width;
                                for (int t = 0; t < tileCellMaxWidth; t++) {
                                    // Loop or cut off sourceTileCellX
                                    sourceTileCellX = ix;
                                    if (layer.Flags & SceneLayer::FLAGS_NO_REPEAT_X) {
                                        if (sourceTileCellX < 0) goto SKIP_TILE_DRAW;
                                        if (sourceTileCellX >= layer.Width) goto SKIP_TILE_DRAW;
                                    }
                                    else {
                                        // sourceTileCellX = ((sourceTileCellX % layer.Width) + layer.Width) % layer.Width;
                                        while (sourceTileCellX < 0) sourceTileCellX += layer.Width;
                                        while (sourceTileCellX >= layer.Width) sourceTileCellX -= layer.Width;
                                    }

                                    tileOrig = tile = layer.Tiles[sourceTileCellX + (sourceTileCellY << layer.WidthInBits)];

                                    tile &= TILE_IDENT_MASK;
                                    if (tile != EmptyTile) {
                                        flipX = (tileOrig & TILE_FLIPX_MASK);
                                        flipY = (tileOrig & TILE_FLIPY_MASK);

                                        int partY = TileBaseY & 0xF;
                                        if (flipY) partY = tileSize - height - partY;

                                        Graphics::DrawSpritePart(TileSprite, 0, tile, 0, partY, tileSize, height, baseX, baseY, flipX, flipY, 1.0f, 1.0f, 0.0f);

                                        if (Scene::ShowTileCollisionFlag && Scene::TileCfgA && layer.ScrollInfoCount <= 1) {
                                            col = 0;
                                            if (Scene::ShowTileCollisionFlag == 1)
                                                col = (tileOrig & TILE_COLLA_MASK) >> 28;
                                            else if (Scene::ShowTileCollisionFlag == 2)
                                                col = (tileOrig & TILE_COLLB_MASK) >> 26;

                                            switch (col) {
                                                case 1:
                                                    Graphics::SetBlendColor(1.0, 1.0, 0.0, 1.0);
                                                    break;
                                                case 2:
                                                    Graphics::SetBlendColor(1.0, 0.0, 0.0, 1.0);
                                                    break;
                                                case 3:
                                                    Graphics::SetBlendColor(1.0, 1.0, 1.0, 1.0);
                                                    break;
                                            }

                                            int xorFlipX = 0;
                                            if (flipX)
                                                xorFlipX = tileSize - 1;

                                            if (baseTileCfg[tile].IsCeiling ^ flipY) {
                                                for (int checkX = 0, realCheckX = 0; checkX < tileSize; checkX++) {
                                                    realCheckX = checkX ^ xorFlipX;
                                                    if (!baseTileCfg[tile].HasCollision[realCheckX]) continue;

                                                    Uint8 colH = baseTileCfg[tile].Collision[realCheckX];
                                                    Graphics::FillRectangle(baseX - 8 + checkX, baseY - 8, 1, tileSize - colH);
                                                }
                                            }
                                            else {
                                                for (int checkX = 0, realCheckX = 0; checkX < tileSize; checkX++) {
                                                    realCheckX = checkX ^ xorFlipX;
                                                    if (!baseTileCfg[tile].HasCollision[realCheckX]) continue;

                                                    Uint8 colH = baseTileCfg[tile].Collision[realCheckX];
                                                    Graphics::FillRectangle(baseX - 8 + checkX, baseY - 8 + colH, 1, tileSize - colH);
                                                }
                                            }
                                        }
                                    }

                                    SKIP_TILE_DRAW:
                                    ix++;
                                    baseX += tileSize;
                                }

                                SKIP_TILE_ROW_DRAW:
                                TileBaseY += height;
                            }
                        }
                        else {
                            baseXOff = (int)std::floor(currentView->X);
                            baseYOff = (int)std::floor(currentView->Y);
                            baseXOff += layer.OffsetX;
                            baseYOff += layer.OffsetY;

                            TileBaseX = baseXOff;
                            TileBaseY = baseYOff;

                            if (Graphics::SupportsBatching && Application::Platform == Platforms::Android && false) { // && Scene::UseBatchedTiles
                                int xxx = baseXOff % layerWidth;
                                int yyy = baseYOff % layerHeight;
                                if (xxx < 0) xxx += layerWidth;
                                if (yyy < 0) yyy += layerHeight;

                                Graphics::Save();
                                Graphics::Translate(-xxx, -yyy, 0.0f);
                                    GLRenderer::DrawTexturedShapeBuffer(Scene::TileSprite->Spritesheets[0], layer.BufferID, layer.VertexCount);
                                    if (!(layer.Flags & SceneLayer::FLAGS_NO_REPEAT_X)) {
                                        Graphics::Translate(layerWidth, 0.0f, 0.0f);
                                        GLRenderer::DrawTexturedShapeBuffer(Scene::TileSprite->Spritesheets[0], layer.BufferID, layer.VertexCount);
                                    }
                                    if (layerWidth < currentView->Width) {
                                        Graphics::Translate(layerWidth, 0.0f, 0.0f);
                                        GLRenderer::DrawTexturedShapeBuffer(Scene::TileSprite->Spritesheets[0], layer.BufferID, layer.VertexCount);
                                    }
                                Graphics::Restore();

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
                                sourceTileCellX &= layer.WidthMask; // sourceTileCellX = ((sourceTileCellX % layer.Width) + layer.Width) % layer.Width;

                                // while (sourceTileCellY < 0) sourceTileCellY += layer.Height;
                                // while (sourceTileCellY >= layer.Height) sourceTileCellY -= layer.Height;
                                sourceTileCellY &= layer.HeightMask;

                                baseX = (ix * tileSize) + tileSizeHalf;
                                baseY = (iy * tileSize) + tileSizeHalf;

                                baseX -= baseXOff;
                                baseY -= baseYOff;

                                tile = layer.Tiles[sourceTileCellX + (sourceTileCellY << layer.WidthInBits)];
                                flipX = (tile & TILE_FLIPX_MASK);
                                flipY = (tile & TILE_FLIPY_MASK);

                                col = 0;
                                if (Scene::ShowTileCollisionFlag && Scene::TileCfgA && layer.ScrollInfoCount <= 1) {
                                    if (Scene::ShowTileCollisionFlag == 1)
                                        col = (tile & TILE_COLLA_MASK) >> 28;
                                    else if (Scene::ShowTileCollisionFlag == 2)
                                        col = (tile & TILE_COLLB_MASK) >> 26;
                                }

                                tile &= TILE_IDENT_MASK;
                                if (tile != EmptyTile) {
                                    Graphics::DrawSprite(TileSprite, 0, tile, baseX, baseY, flipX, flipY, 1.0f, 1.0f, 0.0f);

                                    if (col) {
                                        switch (col) {
                                            case 1:
                                                Graphics::SetBlendColor(1.0, 1.0, 0.0, 1.0);
                                                break;
                                            case 2:
                                                Graphics::SetBlendColor(1.0, 0.0, 0.0, 1.0);
                                                break;
                                            case 3:
                                                Graphics::SetBlendColor(1.0, 1.0, 1.0, 1.0);
                                                break;
                                        }

                                        int xorFlipX = 0;
                                        if (flipX)
                                            xorFlipX = tileSize - 1;

                                        if (baseTileCfg[tile].IsCeiling ^ flipY) {
                                            for (int checkX = 0, realCheckX = 0; checkX < tileSize; checkX++) {
                                                realCheckX = checkX ^ xorFlipX;
                                                if (!baseTileCfg[tile].HasCollision[realCheckX]) continue;

                                                Uint8 colH = baseTileCfg[tile].Collision[realCheckX];
                                                Graphics::FillRectangle(baseX - 8 + checkX, baseY - 8, 1, tileSize - colH);
                                            }
                                        }
                                        else {
                                            for (int checkX = 0, realCheckX = 0; checkX < tileSize; checkX++) {
                                                realCheckX = checkX ^ xorFlipX;
                                                if (!baseTileCfg[tile].HasCollision[realCheckX]) continue;

                                                Uint8 colH = baseTileCfg[tile].Collision[realCheckX];
                                                Graphics::FillRectangle(baseX - 8 + checkX, baseY - 8 + colH, 1, tileSize - colH);
                                            }
                                        }
                                    }
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

                    Graphics::Restore();
                }
            }
            Graphics::TextureBlend = texBlend;
        }
        Scene::GameRenderViewRenderTimes[i] = Clock::GetTicks() - Scene::GameRenderViewRenderTimes[i];

        // RenderLate
        Scene::GameRenderLateViewRenderTimes[i] = Clock::GetTicks();
        for (int l = 0; l < Scene::PriorityPerLayer; l++) {
            if (DEV_NoObjectRender)
                break;

            size_t oSz = PriorityLists[l].size();
            for (size_t o = 0; o < oSz; o++) {
                if (PriorityLists[l][o] && PriorityLists[l][o]->Active)
                    PriorityLists[l][o]->RenderLate();
            }
        }
        Scene::GameRenderLateViewRenderTimes[i] = Clock::GetTicks() - Scene::GameRenderLateViewRenderTimes[i];


        Scene::PostGameRenderViewRenderTimes[i] = Clock::GetTicks();
        if (currentView->UseDrawTarget && currentView->DrawTarget) {
            if (currentView->Software)
                Graphics::SoftwareEnd();

            Graphics::SetRenderTarget(NULL);
            if (currentView->Visible) {
                Graphics::UpdateOrthoFlipped(win_w, win_h);
                Graphics::UpdateProjectionMatrix();

                float out_x = 0.0f;
                float out_y = 0.0f;
                float out_w, out_h;
                switch (1) {
                    // Stretch
                    case 0:
                        out_w = win_w;
                        out_h = win_h;
                        break;
                    // Fit to aspect ratio
                    case 1:
                        if (win_w / currentView->Width < win_h / currentView->Height) {
                            out_w = win_w;
                            out_h = win_w * currentView->Height / currentView->Width;
                        }
                        else {
                            out_w = win_h * currentView->Width / currentView->Height;
                            out_h = win_h;
                        }
                        out_x = (win_w - out_w) * 0.5f;
                        out_y = (win_h - out_h) * 0.5f;
                        break;
                    // Fill to aspect ratio
                    case 2:
                        if (win_w / currentView->Width > win_h / currentView->Height) {
                            out_w = win_w;
                            out_h = win_w * currentView->Height / currentView->Width;
                        }
                        else {
                            out_w = win_h * currentView->Width / currentView->Height;
                            out_h = win_h;
                        }
                        out_x = (win_w - out_w) * 0.5f;
                        out_y = (win_h - out_h) * 0.5f;
                        break;
                }

                Graphics::TextureBlend = false;
                Graphics::SetBlendMode(
                    BlendFactor_SRC_ALPHA, BlendFactor_INV_SRC_ALPHA,
                    BlendFactor_SRC_ALPHA, BlendFactor_INV_SRC_ALPHA);
                Graphics::DrawTexture(currentView->DrawTarget,
                    0.0, 0.0, currentView->Width, currentView->Height,
                    out_x, out_y + Graphics::PixelOffset, out_w, out_h + Graphics::PixelOffset);
            }
        }
        Scene::PostGameRenderViewRenderTimes[i] = Clock::GetTicks() - Scene::PostGameRenderViewRenderTimes[i];
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
						// 	Graphics::DrawSprite(AnimTileSprite, Data->isAnims[tile] >> 8, Data->animatedTileFrames[anID], baseX + 8, baseY + 8, 0, fullFlip, 1.0f, 1.0f, 0.0f);
						// else
							Graphics::DrawSpritePart(TileSprite, 0, tile, 0, wheree, tileSize, 1, baseX + tileSizeHalf, baseY + tileSizeHalf, fullFlip & 1, false, 1.0f, 1.0f, 0.0f);
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
						// 	Graphics::DrawSprite(AnimTileSprite, Data->isAnims[tile] >> 8, Data->animatedTileFrames[anID], baseX + 8, baseY + 8, 0, fullFlip, 1.0f, 1.0f, 0.0f);
						// else
							Graphics::DrawSprite(TileSprite, 0, tile, baseX + tileSizeHalf, baseY + tileSizeHalf, flipX, flipY, 1.0f, 1.0f, 0.0f);
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
        BytecodeObjectManager::ForceGarbageCollection();

        Scene::LoadScene(Scene::NextScene);
        Scene::Restart();

        Scene::NextScene[0] = '\0';
        Scene::DoRestart = false;
    }
    if (Scene::DoRestart) {
        // BytecodeObjectManager::ForceGarbageCollection();

        Scene::Restart();
        Scene::DoRestart = false;
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

    if (Scene::AnyLayerTileChange) {
        // Copy backup tiles into main tiles
        for (int l = 0; l < (int)Layers.size(); l++) {
            memcpy(Layers[l].Tiles, Layers[l].TilesBackup, (Layers[l].WidthMask + 1) * (Layers[l].HeightMask + 1) * sizeof(Uint32));
        }
        Scene::AnyLayerTileChange = false;
    }
    Scene::UpdateTileBatchAll();

    if (Scene::PriorityLists) {
        for (int l = 0; l < Scene::PriorityPerLayer; l++) {
            Scene::PriorityLists[l].clear();
        }
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
        ent->Create(0);
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

    // Dispose of resources in SCOPE_SCENE
    AudioManager::Lock();
    Scene::DisposeInScope(SCOPE_SCENE);
    AudioManager::Unlock();

    // Clear and dispose of objects
    // TODO: Alter this for persistency
    for (Entity* ent = Scene::StaticObjectFirst, *next; ent; ent = next) {
        next = ent->NextEntity;
        if (!ent->Persistent) {
            ent->Dispose();
            // Scene::Remove(&Scene::StaticObjectFirst, &Scene::StaticObjectLast, &Scene::StaticObjectCount, ent);
        }
    }
    Scene::Clear(&Scene::StaticObjectFirst, &Scene::StaticObjectLast, &Scene::StaticObjectCount);

    for (Entity* ent = Scene::DynamicObjectFirst, *next; ent; ent = next) {
        next = ent->NextEntity;
        if (!ent->Persistent) {
            ent->Dispose();
            // Scene::Remove(&Scene::DynamicObjectFirst, &Scene::DynamicObjectLast, &Scene::DynamicObjectCount, ent);
        }
    }
    Scene::Clear(&Scene::DynamicObjectFirst, &Scene::DynamicObjectLast, &Scene::DynamicObjectCount);

    // Clear PriorityLists
    if (Scene::PriorityLists) {
        int layerSize = Scene::PriorityPerLayer;
        for (int l = 0; l < layerSize; l++) {
            Scene::PriorityLists[l].clear();
        }
    }

    // Dispose of layers
    for (size_t i = 0; i < Scene::Layers.size(); i++) {
        Scene::Layers[i].Dispose();
    }
    Scene::Layers.clear();

    // Dispose of TileConfigs
    if (Scene::TileCfgA) {
        for (int i = 0; i < Scene::TileCount; i++) {
            Memory::Free(Scene::TileCfgA[i].Collision);
            Memory::Free(Scene::TileCfgA[i].HasCollision);
            Memory::Free(Scene::TileCfgB[i].Collision);
            Memory::Free(Scene::TileCfgB[i].HasCollision);
        }
        Memory::Free(Scene::TileCfgA);
    }
    Scene::TileCfgA = NULL;
    Scene::TileCfgB = NULL;

    // Force garbage collect
    BytecodeObjectManager::ResetStack();
    BytecodeObjectManager::ForceGarbageCollection();
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

    // Add "Static" class
    if (Application::GameStart) {
        Entity* obj = NULL;
        const char* objectName;
        Uint32 objectNameHash;
        ObjectList* objectList;

        objectName = "Static";
        objectNameHash = CombinedHash::EncryptString(objectName);
        objectList = new ObjectList();
        strcpy(objectList->ObjectName, objectName);
        objectList->SpawnFunction = (Entity*(*)())BytecodeObjectManager::GetSpawnFunction(objectNameHash, (char*)objectName);
        // Scene::ObjectLists->Put(objectNameHash, objectList);

        if (objectList->SpawnFunction) {
            obj = objectList->SpawnFunction();
            if (obj) {
                obj->X = 0.0f;
                obj->Y = 0.0f;
                obj->InitialX = obj->X;
                obj->InitialY = obj->Y;
                obj->List = objectList;
                obj->Persistent = true;
                // Scene::AddStatic(objectList, obj);

                BytecodeObjectManager::Globals->Put("global", OBJECT_VAL(((BytecodeObject*)obj)->Instance));

                if (Application::GameStart) {
                    if (StaticObject)
                        StaticObject->Dispose();

                    obj->GameStart();
                    Application::GameStart = false;
                }
            }
        }

        StaticObject = obj;
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

        Uint8* tileInfo = (Uint8*)Memory::Calloc(1, tileCount * 2 * 0x26);
        tileColReader->ReadCompressed(tileInfo);

        Scene::TileSize = 16;
        Scene::TileCount = tileCount;
        if (Scene::TileCfgA == NULL) {
            Scene::TileCfgA = (TileConfig*)Memory::TrackedCalloc("Scene::TileCfgA", (Scene::TileCount << 1), sizeof(TileConfig));
            Scene::TileCfgB = Scene::TileCfgA + Scene::TileCount;

            for (Uint32 i = 0, iSz = (Uint32)Scene::TileCount; i < iSz; i++) {
                Scene::TileCfgA[i].Collision = (Uint8*)Memory::Calloc(1, Scene::TileSize);
                Scene::TileCfgA[i].HasCollision = (Uint8*)Memory::Calloc(1, Scene::TileSize);
                Scene::TileCfgB[i].Collision = (Uint8*)Memory::Calloc(1, Scene::TileSize);
                Scene::TileCfgB[i].HasCollision = (Uint8*)Memory::Calloc(1, Scene::TileSize);
            }
        }

        Uint8* line;
        for (Uint32 i = 0; i < tileCount; i++) {
            // line = &tileInfo[(i - 1) * 0x26];
            // if (i == 0)
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
            // line = &tileInfo[(tileCount + (i - 1)) * 0x26];
            // if (i == 0)
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
    else if (magic == 0x4C4F4354U) {
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

        if (TileSprite && Scene::TileCount < (int)TileSprite->Animations[0].Frames.size()) {
            Log::Print(Log::LOG_ERROR, "Less Tile Collisions (%d) than actual Tiles! (%d)", Scene::TileCount, (int)TileSprite->Animations[0].Frames.size());
            exit(0);
        }

        for (Uint32 i = 0; i < tileCount; i++) {
            Scene::TileCfgA[i].IsCeiling = Scene::TileCfgB[i].IsCeiling = tileColReader->ReadByte();
            Scene::TileCfgA[i].Config[0] = Scene::TileCfgB[i].Config[0] = tileColReader->ReadByte();
            Scene::TileCfgA[i].Config[1] = false;
            bool hasCollision = tileColReader->ReadByte();
            for (int t = 0; t < tileSize; t++) {
                Scene::TileCfgA[i].Collision[t] = Scene::TileCfgB[i].Collision[t] = tileColReader->ReadByte(); // collision
                Scene::TileCfgA[i].HasCollision[t] = Scene::TileCfgB[i].HasCollision[t] = hasCollision; // (Scene::TileCfgA[i].Collision[t] != tileSize)
            }

            Uint8 angle = Scene::TileCfgA[i].Config[0];

            if (angle == 0xFF) {
                Scene::TileCfgA[i].Config[0] = 0x00; // Top
                Scene::TileCfgA[i].Config[1] = 0xC0; // Left
                Scene::TileCfgA[i].Config[2] = 0x40; // Right
                Scene::TileCfgA[i].Config[3] = 0x80; // Bottom
            }
            else {
                if (Scene::TileCfgA[i].IsCeiling) {
                    Scene::TileCfgA[i].Config[0] = 0x00;
                    Scene::TileCfgA[i].Config[1] = angle >= 0x81 && angle <= 0xB6 ? angle : 0xC0;
                    Scene::TileCfgA[i].Config[2] = angle >= 0x4A && angle <= 0x7F ? angle : 0x40;
                    Scene::TileCfgA[i].Config[3] = angle;
                }
                else {
                    Scene::TileCfgA[i].Config[0] = angle;
                    Scene::TileCfgA[i].Config[1] = angle >= 0xCA && angle <= 0xF6 ? angle : 0xC0;
                    Scene::TileCfgA[i].Config[2] = angle >= 0x0A && angle <= 0x36 ? angle : 0x40;
                    Scene::TileCfgA[i].Config[3] = 0x80;
                }
            }

            // Copy over to B
            memcpy(Scene::TileCfgB[i].Config, Scene::TileCfgA[i].Config, 5);
        }
    }
    else {
        Log::Print(Log::LOG_ERROR, "Invalid magic for TileCollisions! %X", magic);
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

PUBLIC STATIC void Scene::DisposeInScope(Uint32 scope) {
    // Images
    for (size_t i = 0, i_sz = Scene::ImageList.size(); i < i_sz; i++) {
        if (!Scene::ImageList[i]) continue;
        if (Scene::ImageList[i]->UnloadPolicy > scope) continue;

        Scene::ImageList[i]->AsImage->Dispose();
        delete Scene::ImageList[i]->AsImage;
        Scene::ImageList[i] = NULL;
    }
    // Sprites
    for (size_t i = 0, i_sz = Scene::SpriteList.size(); i < i_sz; i++) {
        if (!Scene::SpriteList[i]) continue;
        if (Scene::SpriteList[i]->UnloadPolicy > scope) continue;

        Scene::SpriteList[i]->AsSprite->Dispose();
        delete Scene::SpriteList[i]->AsSprite;
        Scene::SpriteList[i] = NULL;
    }
    // Sounds
    // AudioManager::AudioPauseAll();
    AudioManager::ClearMusic();
    AudioManager::AudioStopAll();
    for (size_t i = 0, i_sz = Scene::SoundList.size(); i < i_sz; i++) {
        if (!Scene::SoundList[i]) continue;
        if (Scene::SoundList[i]->UnloadPolicy > scope) continue;

        Scene::SoundList[i]->AsSound->Dispose();
        delete Scene::SoundList[i]->AsSound;
        Scene::SoundList[i] = NULL;
    }
    // Music
    for (size_t i = 0, i_sz = Scene::MusicList.size(); i < i_sz; i++) {
        if (!Scene::MusicList[i]) continue;
        if (Scene::MusicList[i]->UnloadPolicy > scope) continue;

        // AudioManager::RemoveMusic(Scene::MusicList[i]->AsMusic);

        Scene::MusicList[i]->AsMusic->Dispose();
        delete Scene::MusicList[i]->AsMusic;
        Scene::MusicList[i] = NULL;
    }
    // Media
    for (size_t i = 0, i_sz = Scene::MediaList.size(); i < i_sz; i++) {
        if (!Scene::MediaList[i]) continue;
        if (Scene::MediaList[i]->UnloadPolicy > scope) continue;

        #ifndef NO_LIBAV
        Scene::MediaList[i]->AsMedia->Player->Close();
        Scene::MediaList[i]->AsMedia->Source->Close();
        #endif
        delete Scene::MediaList[i]->AsMedia;

        Scene::MediaList[i] = NULL;
    }
}
PUBLIC STATIC void Scene::Dispose() {
    for (int i = 0; i < 8; i++) {
        if (Scene::Views[i].DrawTarget) {
            Graphics::DisposeTexture(Scene::Views[i].DrawTarget);
            Scene::Views[i].DrawTarget = NULL;
        }

        if (Scene::Views[i].ProjectionMatrix) {
            delete Scene::Views[i].ProjectionMatrix;
            Scene::Views[i].ProjectionMatrix = NULL;
        }

        if (Scene::Views[i].BaseProjectionMatrix) {
            delete Scene::Views[i].BaseProjectionMatrix;
            Scene::Views[i].BaseProjectionMatrix = NULL;
        }
    }

    AudioManager::Lock();
    // Make sure no audio is playing (really we don't want any audio to be playing, otherwise we'll get a EXC_BAD_ACCESS)
    AudioManager::ClearMusic();
    AudioManager::AudioStopAll();

    Scene::DisposeInScope(SCOPE_GAME);
    // Dispose of all resources
    Scene::ImageList.clear();
    Scene::SpriteList.clear();
    Scene::SoundList.clear();
    Scene::MusicList.clear();
    Scene::MediaList.clear();

    AudioManager::Unlock();

    if (StaticObject)
        StaticObject->Dispose();

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
    if (Scene::PriorityLists)
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
        for (int i = 0; i < Scene::TileCount; i++) {
            Memory::Free(Scene::TileCfgA[i].Collision);
            Memory::Free(Scene::TileCfgA[i].HasCollision);
            Memory::Free(Scene::TileCfgB[i].Collision);
            Memory::Free(Scene::TileCfgB[i].HasCollision);
        }
        Memory::Free(Scene::TileCfgA);
    }
    Scene::TileCfgA = NULL;
    Scene::TileCfgB = NULL;

    BytecodeObjectManager::Dispose();
    SourceFileMap::Dispose();
}

PUBLIC STATIC void Scene::Exit() {

}

// Tile Batching
PUBLIC STATIC void Scene::UpdateTileBatchAll() {
    for (size_t l = 0; l < Scene::Layers.size(); l++) {
        SceneLayer* layer = &Scene::Layers[l];
        int batch_count_x = (layer->Width + TileBatchMaxSize - 1) / TileBatchMaxSize;
        int batch_count_y = (layer->Height + TileBatchMaxSize - 1) / TileBatchMaxSize;
        for (int x = 0; x < batch_count_x; x++) {
            for (int y = 0; y < batch_count_y; y++) {
                UpdateTileBatch(l, x, y);
            }
        }
    }
    /*
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

            tileID = tSauce & TILE_IDENT_MASK;
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
    //*/
}
PUBLIC STATIC void Scene::UpdateTileBatch(int l, int batchx, int batchy) {
    // if (!Scene::UseBatchedTiles)
    //     return;

    if (!Graphics::SupportsBatching || !Scene::TileSprite)
        return;

    SceneLayer* layer = &Scene::Layers[l];
    int batch_count_x = (layer->Width + TileBatchMaxSize - 1) / TileBatchMaxSize;
    int batch_count_y = (layer->Height + TileBatchMaxSize - 1) / TileBatchMaxSize;
    if (!layer->TileBatches) {
        layer->TileBatches = Memory::Malloc(batch_count_x * batch_count_y * sizeof(TileBatch));
    }

    float spriteW = Scene::TileSprite->Spritesheets[0]->Width;
    float spriteH = Scene::TileSprite->Spritesheets[0]->Height;
    float tileSize = (float)Scene::TileSize;

    struct tempVertex {
        float x;
        float y;
        float z;
        float u;
        float v;
    };
    tempVertex* buffer = (tempVertex*)Memory::Malloc(TileBatchMaxSize * TileBatchMaxSize * sizeof(tempVertex) * 6);

    int    vertexCount = 0;
    int    tStartX = batchx * TileBatchMaxSize;
    int    tStartY = batchy * TileBatchMaxSize;
    int    tEndX = tStartX + TileBatchMaxSize;
    int    tEndY = tStartY + TileBatchMaxSize;
    int    tSauce, tileID, flipX, flipY, tx, ty, srcx, srcy;
    float  left, right, top, bottom, posX, posY, posXW, posYH;

    if (tEndX >= layer->Width)
        tEndX  = layer->Width;
    if (tEndY >= layer->Height)
        tEndY  = layer->Height;

    for (tx = tStartX; tx < tEndX; tx++) {
        int t = tx + tStartY * layer->Width;
        for (ty = tStartY; ty < tEndY; t += layer->Width, ty++) {
            if ((layer->Tiles[t] & TILE_IDENT_MASK) == EmptyTile) continue;

            tSauce = layer->Tiles[t];

            tileID = tSauce & TILE_IDENT_MASK;
            flipX  = tSauce & TILE_FLIPX_MASK;
            flipY  = tSauce & TILE_FLIPY_MASK;

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
    }

    TileBatch* tileBatch = &((TileBatch*)layer->TileBatches)[batchx + batchy * batch_count_x];
    tileBatch->BufferID = GLRenderer::CreateTexturedShapeBuffer((float*)buffer, vertexCount);
    // tileBatch->BufferID = D3DRenderer::CreateTexturedShapeBuffer((float**)&buffer, vertexCount);
    tileBatch->VertexCount = vertexCount;

    Memory::Free(buffer);
}
PUBLIC STATIC void Scene::SetTile(int layer, int x, int y, int tileID, int flip_x, int flip_y, int collA, int collB) {
    Uint32* tile = &Scene::Layers[layer].Tiles[x + (y << Scene::Layers[layer].WidthInBits)];

    *tile = tileID & TILE_IDENT_MASK;
    if (flip_x)
        *tile |= TILE_FLIPX_MASK;
    if (flip_y)
        *tile |= TILE_FLIPY_MASK;
    *tile |= collA << 28;
    *tile |= collB << 26;

    UpdateTileBatch(layer, x / TileBatchMaxSize, y / TileBatchMaxSize);
}

// Tile Collision
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
        if (x < 0 || x >= temp)
            continue;
        x &= layer.WidthMask << 4 | 0xF; // x = ((x % temp) + temp) % temp;

        // Check Layer Height
        temp = layer.Height << 4;
        if ((y < 0 || y >= temp))
            continue;
        y &= layer.HeightMask << 4 | 0xF; // y = ((y % temp) + temp) % temp;

        tileX = x >> 4;
        tileY = y >> 4;

        tileID = layer.Tiles[tileX + (tileY << layer.WidthInBits)];
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
            check = ((collision & 1) && (collideSide & CollideSide::TOP)) ||
                (wallAsFloorFlag && ((collision & 1) && (collideSide & (CollideSide::LEFT | CollideSide::RIGHT)))) ||
                ((collision & 2) && (collideSide & CollideSide::BOTTOM_SIDES));
            if (!check)
                continue;

            // Check Y
            tileY = tileY << 4;
            check = ((isCeiling ^ flipY) && (y >= tileY && y < tileY + tileSize - height)) ||
                    (!(isCeiling ^ flipY) && (y >= tileY + height && y < tileY + tileSize));
            if (!check)
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
// CollisionInLine
