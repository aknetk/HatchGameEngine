#ifndef ENGINE_SCENE_H
#define ENGINE_SCENE_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED

class Entity;

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

    static void Add(Entity** first, Entity** last, int* count, Entity* obj);
    static void Remove(Entity** first, Entity** last, int* count, Entity* obj);
    static void Clear(Entity** first, Entity** last, int* count);
    static void AddStatic(ObjectList* objectList, Entity* obj);
    static void AddDynamic(ObjectList* objectList, Entity* obj);
    static void OnEvent(Uint32 event);
    static void Init();
    static void Update();
    static void Render();
    static void AfterScene();
    static void Restart();
    static void LoadScene(const char* filename);
    static void LoadTileCollisions(const char* filename);
    static void SaveTileCollisions();
    static void DisposeInScope(Uint32 scope);
    static void Dispose();
    static void Exit();
    static void UpdateTileBatchAll();
    static void UpdateTileBatch(int l, int batchx, int batchy);
    static void SetTile(int layer, int x, int y, int tileID, int flip_x, int flip_y, int collA, int collB);
    static int  CollisionAt(int x, int y, int collisionField, int collideSide, int* angle);
};

#endif /* ENGINE_SCENE_H */
