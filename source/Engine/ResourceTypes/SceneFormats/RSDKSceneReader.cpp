#if INTERFACE
#include <Engine/IO/Stream.h>
class RSDKSceneReader {
public:
    static bool             Initialized;
};
#endif

#include <Engine/ResourceTypes/SceneFormats/RSDKSceneReader.h>

#include <Engine/IO/MemoryStream.h>
#include <Engine/Bytecode/BytecodeObjectManager.h>
#include <Engine/Bytecode/BytecodeObject.h>
#include <Engine/Bytecode/Compiler.h>
#include <Engine/Diagnostics/Clock.h>
#include <Engine/Diagnostics/Log.h>
#include <Engine/Diagnostics/Memory.h>
#include <Engine/Hashing/FNV1A.h>
#include <Engine/Hashing/CombinedHash.h>
#include <Engine/Hashing/CRC32.h>
#include <Engine/IO/Compression/ZLibStream.h>
#include <Engine/IO/ResourceStream.h>
#include <Engine/Includes/HashMap.h>
#include <Engine/Rendering/Software/SoftwareRenderer.h>
#include <Engine/ResourceTypes/ImageFormats/GIF.h>
#include <Engine/ResourceTypes/ResourceManager.h>
#include <Engine/Scene/SceneLayer.h>
#include <Engine/Scene.h>

#include <Engine/TextFormats/XML/XMLParser.h>

#define TILE_FLIPX_MASK 0x80000000U
#define TILE_FLIPY_MASK 0x40000000U
#define TILE_COLLA_MASK 0x30000000U
#define TILE_COLLB_MASK 0x0C000000U
#define TILE_COLLC_MASK 0x03000000U
#define TILE_IDENT_MASK 0x00FFFFFFU

bool            RSDKSceneReader::Initialized = false;
char*           ObjectNames = NULL;
char*           PropertyNames = NULL;
HashMap<const char*>* ObjectHashes = NULL;
HashMap<const char*>* PropertyHashes = NULL;

void StageConfig_GetColors(const char* filename) {
    MemoryStream* memoryReader;
    ResourceStream* stageConfigReader;
    if ((stageConfigReader = ResourceStream::New(filename))) {
        if ((memoryReader = MemoryStream::New(stageConfigReader))) {
            do {
                Uint32 magic = memoryReader->ReadUInt32();
                if (magic != 0x474643)
                    break;

                memoryReader->ReadByte(); // useGameObjects

                int objectNameCount = memoryReader->ReadByte();
                for (int i = 0; i < objectNameCount; i++)
                    Memory::Free(memoryReader->ReadHeaderedString());

                int paletteCount = 8;

                Uint8 Color[3];
                for (int i = 0; i < paletteCount; i++) {
                    // Palette Set
                    int bitmap = memoryReader->ReadUInt16();
                    for (int col = 0; col < 16; col++) {
                        if ((bitmap & (1 << col)) != 0) {
                            for (int d = 0; d < 16; d++) {
                                memoryReader->ReadBytes(Color, 3);
                                if (Color[0] == 0xFF && Color[1] == 0x00 && Color[2] == 0xFF)
                                    continue;

                                SoftwareRenderer::PaletteColors[i][(col << 4) | d] = 0xFF000000U | Color[0] << 16 | Color[1] << 8 | Color[2];
                            }
                            SoftwareRenderer::ConvertFromARGBtoNative(&SoftwareRenderer::PaletteColors[i][(col << 4)], 16);
                        }
                    }
                }

                int wavConfigCount = memoryReader->ReadByte();
                for (int i = 0; i < wavConfigCount; i++) {
                    Memory::Free(memoryReader->ReadHeaderedString());
                    memoryReader->ReadByte();
                }
            } while (false);

            memoryReader->Close();
        }
        stageConfigReader->Close();
    }
}
void GameConfig_GetColors(const char* filename) {
    MemoryStream* memoryReader;
    ResourceStream* gameConfigReader;
    if ((gameConfigReader = ResourceStream::New(filename))) {
        if ((memoryReader = MemoryStream::New(gameConfigReader))) {
            do {
                Uint32 magic = memoryReader->ReadUInt32();
                if (magic != 0x474643)
                    break;

                Memory::Free(memoryReader->ReadHeaderedString());
                Memory::Free(memoryReader->ReadHeaderedString());
                Memory::Free(memoryReader->ReadHeaderedString());

                memoryReader->ReadByte(); // useGameObjects
                memoryReader->ReadUInt16();

                // Common config
                int objectNameCount = memoryReader->ReadByte();
                for (int i = 0; i < objectNameCount; i++)
                    Memory::Free(memoryReader->ReadHeaderedString());

                int paletteCount = 8;

                Uint8 Color[3];
                for (int i = 0; i < paletteCount; i++) {
                    // Palette Set
                    int bitmap = memoryReader->ReadUInt16();
                    for (int col = 0; col < 16; col++) {
                        if ((bitmap & (1 << col)) != 0) {
                            for (int d = 0; d < 16; d++) {
                                memoryReader->ReadBytes(Color, 3);
                                if (Color[0] == 0xFF && Color[1] == 0x00 && Color[2] == 0xFF)
                                    continue;

                                SoftwareRenderer::PaletteColors[i][(col << 4) | d] = 0xFF000000U | Color[0] << 16 | Color[1] << 8 | Color[2];
                            }
                            SoftwareRenderer::ConvertFromARGBtoNative(&SoftwareRenderer::PaletteColors[i][(col << 4)], 16);
                        }
                    }
                }

                int wavConfigCount = memoryReader->ReadByte();
                for (int i = 0; i < wavConfigCount; i++) {
                    Memory::Free(memoryReader->ReadHeaderedString());
                    memoryReader->ReadByte();
                }
            } while (false);

            memoryReader->Close();
        }
        gameConfigReader->Close();
    }
}

PUBLIC STATIC bool RSDKSceneReader::Read(const char* filename, const char* parentFolder) {
    Stream* r;
    double ticks;
    // char* objectName;
    ObjectList* objectList;
    Uint32 objectNameHash, objectNameHash2, layerCount;
    Uint32 HACK_PlayerNameHash;

    int    roundedUpPow2, objectDefinitionCount;

    Uint8  hashTemp[16];
    int    argumentTypes[0x10];
    Uint32 argumentHashes[0x10];
    int    argumentCount;
    int    entityCount;
    int      maxObjSlots;
    Entity** objSlots = NULL;
    char*    objNames = NULL;

    // Load PropertyList and ObjectList and others
    // (use regular malloc and calloc as this is technically a hack outside the scope of the engine)
    if (!RSDKSceneReader::Initialized) {
        switch (0) {
            default:
            size_t sz;
            char* nameHead;
            char* nameStart;

            // Object names
            {
                r = ResourceStream::New("ObjectList.txt");
                if (!r) break;

                sz = r->Length();
                ObjectNames = (char*)malloc(sz + 1);
                r->ReadBytes(ObjectNames, sz);
                ObjectNames[sz] = 0;

                ObjectHashes = new HashMap<const char*>(CombinedHash::EncryptData, 1024);

                nameHead = ObjectNames;
                nameStart = ObjectNames;
                while (*nameHead) {
                    if (*nameHead == '\n') {
                        *nameHead = 0;

                        if (strcmp(nameStart, "Blank Object") == 0)
                            ObjectHashes->Put(nameStart, "Blank_Object");
                        else if (strcmp(nameStart, "Music") == 0)
                            ObjectHashes->Put(nameStart, "MusicObject");
                        else
                            ObjectHashes->Put(nameStart, nameStart);

                        nameHead++;
                        nameStart = nameHead;
                        continue;
                    }
                    nameHead++;
                }
                ObjectHashes->Put(nameStart, nameStart);

                r->Close();
            }

            // Property Names
            {
                r = ResourceStream::New("PropertyList.txt");
                if (!r) break;

                sz = r->Length();
                PropertyNames = (char*)malloc(sz + 1);
                r->ReadBytes(PropertyNames, sz);
                PropertyNames[sz] = 0;

                PropertyHashes = new HashMap<const char*>(CombinedHash::EncryptData, 512);

                nameHead = PropertyNames;
                nameStart = PropertyNames;
                while (*nameHead) {
                    if (*nameHead == '\n') {
                        *nameHead = 0;
                        PropertyHashes->Put(nameStart, nameStart);
                        nameHead++;
                        nameStart = nameHead;
                        continue;
                    }
                    nameHead++;
                }
                PropertyHashes->Put(nameStart, nameStart);

                r->Close();
            }
        }
        RSDKSceneReader::Initialized = true;
    }

    if (!ObjectNames) //  || !PropertyNames
        return false;

    char stageConfigFilename[256];
    sprintf(stageConfigFilename, "%sStageConfig.bin", parentFolder);
    if (ResourceManager::ResourceExists(stageConfigFilename))
        StageConfig_GetColors(stageConfigFilename);
    else
        Log::Print(Log::LOG_WARN, "No StageConfig at '%s'!", stageConfigFilename);

    r = ResourceStream::New(filename);
    if (!r) {
        Log::Print(Log::LOG_ERROR, "Couldn't open file '%s'!", filename);
		goto FREE;
    }

    // Clear scene to defaults
    Scene::TileSize = 16;
    Scene::EmptyTile = 0x3FF;

    if (r->ReadUInt32BE() != 0x53434E00)
        goto FREE;

    r->Skip(16); // 16 bytes
    r->Skip(r->ReadByte()); // RSDKString

    r->ReadByte();

    ticks = Clock::GetTicks();

    layerCount = r->ReadByte();
    Scene::Layers.resize(layerCount);
    for (Uint32 i = 0; i < layerCount; i++) {
        r->ReadByte(); // Ignored Byte

        char* Name = r->ReadHeaderedString();
        r->ReadByte(); // int   IsScrollingVertical = r->ReadByte() == 1 ? true : false;
        int   DrawGroup = r->ReadByte();
        int   Width = (int)r->ReadUInt16();
        int   Height = (int)r->ReadUInt16();

        SceneLayer layer(Width, Height);

        memset(layer.Name, 0, 50);
        strcpy(layer.Name, Name);
        Memory::Free(Name);

        layer.RelativeY = r->ReadInt16();
        layer.ConstantY = (short)r->ReadInt16();

        layer.Flags = 0;

        if (layer.Name[0] == 'F' && layer.Name[1] == 'G')
            layer.Flags |= SceneLayer::FLAGS_COLLIDEABLE;

        if (strcmp(layer.Name, "Move") == 0)
            layer.Flags |= SceneLayer::FLAGS_COLLIDEABLE | SceneLayer::FLAGS_NO_REPEAT_X | SceneLayer::FLAGS_NO_REPEAT_Y;
        // BUG:
        // Some kinda memory leak when FG High is not immediately after FG Low

        // layer.Flags |= SceneLayer::FLAGS_NO_REPEAT_X | SceneLayer::FLAGS_NO_REPEAT_Y;

        layer.DrawGroup = DrawGroup & 0xF;
        if (DrawGroup & 0x10)
            layer.Visible = false;

        layer.ScrollInfoCount = (int)r->ReadUInt16();
        layer.ScrollInfos = (ScrollingInfo*)Memory::Malloc(layer.ScrollInfoCount * sizeof(ScrollingInfo));
        for (int g = 0; g < layer.ScrollInfoCount; g++) {
            layer.ScrollInfos[g].RelativeX = r->ReadInt16();
            layer.ScrollInfos[g].ConstantX = r->ReadInt16();

            r->ReadByte();
            r->ReadByte();
        }

        Uint16* tileBoys = (Uint16*)malloc(sizeof(Uint16) * Width * Height);

        r->ReadCompressed(layer.ScrollIndexes);
        r->ReadCompressed(tileBoys);

        int height16 = Height * 16;
        int splitCount, lastValue, lastY, sliceH;

        // Count first
        splitCount = 0, lastValue = layer.ScrollIndexes[0], lastY = 0;
        for (int y = 0; y < height16; y++) {
            if ((y > 0 && ((y & 0xF) == 0)) || lastValue != layer.ScrollIndexes[y]) {
                splitCount++;
                lastValue = layer.ScrollIndexes[y];
            }
        }
        splitCount++;

        // printf("%s: rules count %d\n", layer.Name, splitCount);

        layer.ScrollInfosSplitIndexes = (Uint16*)Memory::Malloc(splitCount * sizeof(Uint16));

        splitCount = 0, lastValue = layer.ScrollIndexes[0], lastY = 0;
        for (int y = 0; y < height16; y++) {
            if ((y > 0 && ((y & 0xF) == 0)) || lastValue != layer.ScrollIndexes[y]) {
                // Do slice
                sliceH = y - lastY;
                // if (strcmp(layer.Name, "Background 1") == 0)
                //     printf("rule[%d]: height %d index %d  y: %d\n", splitCount, sliceH, lastValue, y);
                layer.ScrollInfosSplitIndexes[splitCount] = (sliceH << 8) | lastValue;
                splitCount++;

                // Iterate
                lastValue = layer.ScrollIndexes[y];
                lastY = y;
            }
        }

        // Do slice
        sliceH = height16 - lastY;
        layer.ScrollInfosSplitIndexes[splitCount] = (sliceH << 8) | lastValue;
        splitCount++;

        layer.ScrollInfosSplitIndexesCount = splitCount;

        // Convert to HatchTiles
        int t = 0;
        Uint32* tileRow = &layer.Tiles[0];
        for (int y = 0; y < layer.Height; y++) {
            for (int x = 0; x < layer.Width; x++) {
                tileRow[x] = (tileBoys[t] & 0x3FF);
                tileRow[x] |= (tileBoys[t] & 0x400) << 21; // Flip X
                tileRow[x] |= (tileBoys[t] & 0x800) << 19; // Flip Y
                tileRow[x] |= (tileBoys[t] & 0xC000) << 12; // Collision B
                tileRow[x] |= (tileBoys[t] & 0x3000) << 16; // Collision A
                t++;
            }
            tileRow += (layer.WidthMask + 1);
        }
        memcpy(layer.TilesBackup, layer.Tiles, sizeof(Uint32) * (layer.WidthMask + 1) * (layer.HeightMask + 1));

        // Sprite Flag? (Entity -> 0x53)
        // 0x1: Can flip
        // 0x2: Can rotate
        // 0x4: Can scale
        // 0x8: Just fucking disappears

        free(tileBoys);

        Log::Print(Log::LOG_VERBOSE, "Layer %d (%s): Width (%d) Height (%d) DrawGroup (%d)", i, layer.Name, layer.Width, layer.Height, DrawGroup);

        Scene::Layers[i] = layer;
    }

    if (!Scene::PriorityLists) {
        Scene::PriorityLists = (vector<Entity*>*)Memory::TrackedCalloc("Scene::PriorityLists", Scene::PriorityPerLayer, sizeof(vector<Entity*>));
    }

    ticks = Clock::GetTicks() - ticks;
    Log::Print(Log::LOG_VERBOSE, "Scene Layer load took %.3f milliseconds.", ticks);

    ticks = Clock::GetTicks();

    objectDefinitionCount = r->ReadByte();
    Log::Print(Log::LOG_VERBOSE, "Object Definition Count: %d", objectDefinitionCount);

    roundedUpPow2 = objectDefinitionCount;

    roundedUpPow2--;
    roundedUpPow2 |= roundedUpPow2 >> 1;
    roundedUpPow2 |= roundedUpPow2 >> 2;
    roundedUpPow2 |= roundedUpPow2 >> 4;
    roundedUpPow2 |= roundedUpPow2 >> 16;
    roundedUpPow2++;

    if (Scene::ObjectLists == NULL) {
        Scene::ObjectLists = new HashMap<ObjectList*>(CombinedHash::EncryptData, roundedUpPow2);
        Scene::ObjectRegistries = new HashMap<ObjectList*>(CombinedHash::EncryptData, 16);
    }

    // Hack in WindowManager, InputManager, and PauseManager
    {
        Entity* obj;
        const char* objectName;

        objectName = "WindowManager";
        objectNameHash = CombinedHash::EncryptString(objectName);
        objectList = new ObjectList();
        strcpy(objectList->ObjectName, objectName);
        objectList->SpawnFunction = (Entity*(*)())BytecodeObjectManager::GetSpawnFunction(objectNameHash, objectName);
        Scene::ObjectLists->Put(objectNameHash, objectList);

        obj = objectList->SpawnFunction();
        obj->X = 0.0f;
        obj->Y = 0.0f;
        obj->InitialX = obj->X;
        obj->InitialY = obj->Y;
        obj->List = objectList;
        Scene::AddStatic(objectList, obj);

        objectName = "InputManager";
        objectNameHash = CombinedHash::EncryptString(objectName);
        objectList = new ObjectList();
        strcpy(objectList->ObjectName, objectName);
        objectList->SpawnFunction = (Entity*(*)())BytecodeObjectManager::GetSpawnFunction(objectNameHash, objectName);
        Scene::ObjectLists->Put(objectNameHash, objectList);

        obj = objectList->SpawnFunction();
        obj->X = 0.0f;
        obj->Y = 0.0f;
        obj->InitialX = obj->X;
        obj->InitialY = obj->Y;
        obj->List = objectList;
        Scene::AddStatic(objectList, obj);

        objectName = "PauseManager";
        objectNameHash = CombinedHash::EncryptString(objectName);
        objectList = new ObjectList();
        strcpy(objectList->ObjectName, objectName);
        objectList->SpawnFunction = (Entity*(*)())BytecodeObjectManager::GetSpawnFunction(objectNameHash, objectName);
        Scene::ObjectLists->Put(objectNameHash, objectList);

        obj = objectList->SpawnFunction();
        obj->X = 0.0f;
        obj->Y = 0.0f;
        obj->InitialX = obj->X;
        obj->InitialY = obj->Y;
        obj->List = objectList;
        Scene::AddStatic(objectList, obj);
    }

    maxObjSlots = 0x940;
    objSlots = (Entity**)calloc(sizeof(Entity*), maxObjSlots);

    HACK_PlayerNameHash = ObjectHashes->HashFunction("Player", 6);

    for (int i = 0; i < objectDefinitionCount; i++) {
        r->ReadBytes(hashTemp, 16);
        objectNameHash = CRC32::EncryptData(hashTemp, 16);
        objectNameHash2 = objectNameHash;

        if (ObjectHashes->Exists(objectNameHash)) {
            const char* name = ObjectHashes->Get(objectNameHash);
            objectNameHash2 = ObjectHashes->HashFunction(name, strlen(name));
        }

        objectList = new ObjectList();
        strcpy(objectList->ObjectName, ObjectHashes->Get(objectNameHash));
        Scene::ObjectLists->Put(objectNameHash2, objectList);

        argumentCount = r->ReadByte();

        argumentTypes[0] = 8;
        argumentHashes[0] = 0x00000000U;
        for (int a = 1; a < argumentCount; a++) {
            r->ReadBytes(hashTemp, 16);
            argumentTypes[a] = r->ReadByte();
            argumentHashes[a] = CRC32::EncryptData(hashTemp, 16);
        }
        entityCount = r->ReadUInt16();

        if (ObjectHashes->Exists(objectNameHash)) {
            // Log::Print(Log::LOG_VERBOSE, "Object Hash: 0x%08XU (0x%08XU) Count: %*d Argument Count: %d (%s)", objectNameHash, 3, entityCount, argumentCount, ObjectHashes->Get(objectNameHash));

            objectList->SpawnFunction = (Entity*(*)())BytecodeObjectManager::GetSpawnFunction(
                objectNameHash2,
                ObjectHashes->Get(objectNameHash));

            if (objectList->SpawnFunction) {
            //     for (int a = 1; a < argumentCount; a++) {
            //         if (PropertyHashes->Exists(argumentHashes[a]) && argumentHashes[a] != 0x7CEA5BB0U) {
            //             const char* argType;
            //             switch (argumentTypes[a]) {
            //                 case 0: argType = "uint8"; break;
            //                 case 1: argType = "uint16"; break;
            //                 case 2: argType = "uint32"; break;
            //                 case 3: argType = "int8"; break;
            //                 case 4: argType = "int16"; break;
            //                 case 5: argType = "int32"; break;
            //                 case 6: argType = "int32"; break;
            //                 case 7: argType = "bool"; break;
            //                 case 8: argType = "string"; break;
            //                 case 9: argType = "vector2"; break;
            //                 case 11: argType = "color"; break;
            //                 default: argType = "undefined"; break;
            //             }
            //             Log::Print(Log::LOG_VERBOSE, "Property: %s    Type: %s", PropertyHashes->Get(argumentHashes[a]), argType);
            //         }
            //     }
            }
            else {
                Log::Print(Log::LOG_WARN, "Unimplemented %s class!", ObjectHashes->Get(objectNameHash));
            }
        }

        for (int n = 0; n < entityCount; n++) {
            bool doAdd = true;
            int SlotID = r->ReadUInt16();
            if (SlotID >= maxObjSlots) {
                Log::Print(Log::LOG_ERROR, "Too many objects in scene! (Count: %d, Max: %d)", SlotID + 1, maxObjSlots);
                doAdd = false;
                // exit(0);
            }

            Uint32 X = r->ReadUInt32();
            Uint32 Y = r->ReadUInt32();

            if (objectList->SpawnFunction) {
                Entity* obj = objectList->SpawnFunction();
                obj->X = (X / 65536.f);
                obj->Y = (Y / 65536.f);
                obj->InitialX = obj->X;
                obj->InitialY = obj->Y;
                obj->List = objectList;

                // HACK: This is so Player ends up in the current SlotID,
                //       since this currently cannot be changed during runtime.
                if (objectNameHash2 == HACK_PlayerNameHash)
                    Scene::AddStatic(obj->List, obj);
                else if (doAdd)
                    objSlots[SlotID] = obj;

                bool usingBytecodeObjects = true;
                if (usingBytecodeObjects) {
                    for (int a = 1; a < argumentCount; a++) {
                        VMValue val = NULL_VAL;
                        switch (argumentTypes[a]) {
                            case 0x0: val = INTEGER_VAL(r->ReadByte()); break;
                            case 0x1: val = INTEGER_VAL(r->ReadUInt16()); break;
                            case 0x2: val = INTEGER_VAL((int)r->ReadUInt32()); break;
                            case 0x3: val = INTEGER_VAL((Sint8)r->ReadByte()); break;
                            case 0x4: val = INTEGER_VAL(r->ReadInt16()); break;
                            case 0x5: val = INTEGER_VAL(r->ReadInt32()); break;
                            // Var
                            case 0x6: val = INTEGER_VAL(r->ReadInt32()); break;
                            // Bool
                            case 0x7: val = INTEGER_VAL((int)r->ReadUInt32()); break;
                            // String
                            case 0x8: {
                                ObjString* str = AllocString(r->ReadUInt16());
                                for (int c = 0; c < str->Length; c++)
                                    str->Chars[c] = (char)(Uint8)r->ReadUInt16();
                                val = OBJECT_VAL(str);
                                break;
                            }
                            // Position
                            case 0x9: {
                                ObjArray* array = NewArray();
                                // array->Values->push_back(INTEGER_VAL((int)r->ReadUInt32()));
                                // array->Values->push_back(INTEGER_VAL((int)r->ReadUInt32()));
                                array->Values->push_back(DECIMAL_VAL(r->ReadInt32() / 65536.f));
                                array->Values->push_back(DECIMAL_VAL(r->ReadInt32() / 65536.f));
                                val = OBJECT_VAL(array);
                                break;
                            }
                            // Color
                            case 0xB: val = INTEGER_VAL((int)r->ReadUInt32()); break;
                        }

                        if (PropertyHashes->Exists(argumentHashes[a])) {
                            ((BytecodeObject*)obj)->Properties->Put(PropertyHashes->Get(argumentHashes[a]), val);
                        }
                    }
                }
            }
            else {
                int len;
                for (int a = 1; a < argumentCount; a++) {
                    switch (argumentTypes[a]) {
                        case 0x0: r->Skip(1); break;
                        case 0x1: r->Skip(2); break;
                        case 0x2: r->Skip(4); break;
                        case 0x3: r->Skip(1); break;
                        case 0x4: r->Skip(2); break;
                        case 0x5: r->Skip(4); break;
                        // Var
                        case 0x6: r->Skip(4); break;
                        // Bool
                        case 0x7: r->Skip(4); break;
                        // String
                        case 0x8:
                            len = r->ReadUInt16();
                            r->Skip(len << 1);
                            break;
                        // Position
                        case 0x9:
                            r->Skip(8);
                            break;
                        // Color
                        case 0xB: r->Skip(4); break;
                    }
                }
            }
        }
    }

    for (int i = 0; i < maxObjSlots; i++) {
        if (objSlots[i]) {
            Scene::AddStatic(objSlots[i]->List, objSlots[i]);
        }
    }

    ticks = Clock::GetTicks() - ticks;
    Log::Print(Log::LOG_VERBOSE, "Scene Object load took %.3f milliseconds.", ticks);

    FREE:
    if (objSlots)
        free(objSlots);
    r->Close();

    if (Scene::TileSprite) {
        Scene::TileSprite->Dispose();
        delete Scene::TileSprite;
    }

    Scene::TileSprite = new ISprite();

    // Load Tileset and copy palette
    char filename16x16Tiles[256];
    sprintf(filename16x16Tiles, "%s16x16Tiles.gif", parentFolder);
    {
        GIF* gif;
        bool loadPalette = Graphics::UsePalettes;

        Graphics::UsePalettes = false;
        gif = GIF::Load(filename16x16Tiles);
        Graphics::UsePalettes = loadPalette;

        if (gif) {
            if (gif->Colors) {
                for (int p = 0; p < 256; p++)
                    SoftwareRenderer::PaletteColors[0][p] = gif->Colors[p];
                Memory::Free(gif->Colors);
            }
            delete gif;
        }
    }

    int cols, rows;
    Scene::TileSprite->Spritesheets[0] = Scene::TileSprite->AddSpriteSheet(filename16x16Tiles);
    cols = Scene::TileSprite->Spritesheets[0]->Width / Scene::TileSize;
    rows = Scene::TileSprite->Spritesheets[0]->Height / Scene::TileSize;

    Scene::TileSprite->ReserveAnimationCount(1);
    Scene::TileSprite->AddAnimation("TileSprite", 0, 0, cols * rows);
    for (int i = 0; i < cols * rows; i++) {
        Scene::TileSprite->AddFrame(0,
            (i % cols) * Scene::TileSize,
            (i / cols) * Scene::TileSize,
            Scene::TileSize, Scene::TileSize, -Scene::TileSize / 2, -Scene::TileSize / 2);
    }

    // Load GameConfig palettes
    sprintf(stageConfigFilename, "Game/GameConfig.bin", parentFolder);
    if (ResourceManager::ResourceExists(stageConfigFilename))
        GameConfig_GetColors(stageConfigFilename);
    else
        Log::Print(Log::LOG_WARN, "No GameConfig at '%s'!", stageConfigFilename);
    return true;
}
