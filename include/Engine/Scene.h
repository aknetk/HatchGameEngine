#ifndef SCENE_H
#define SCENE_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL

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

    static void Add(Entity** first, Entity** last, int* count, Entity* obj);
    static void Remove(Entity** first, Entity** last, int* count, Entity* obj);
    static void Clear(Entity** first, Entity** last, int* count);
    static void AddStatic(ObjectList* objectList, Entity* obj);
    static void AddDynamic(ObjectList* objectList, Entity* obj);
    static void OnEvent(Uint32 event);
    static void Init();
    static void Restart();
    static void LoadScene(const char* filename);
    static void LoadTileCollisions(const char* filename);
    static void SaveTileCollisions();
    static void Update();
    static void Render();
    static void AfterScene();
    static void Dispose();
    static void Exit();
    static int  CollisionAt(int x, int y, int collisionField, int collideSide, int* angle);
};

#endif /* SCENE_H */
