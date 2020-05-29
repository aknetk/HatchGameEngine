#if INTERFACE
#include <Engine/IO/Stream.h>
class TiledMapReader {
public:
    // static bool             Initialized;
};
#endif

#include <Engine/ResourceTypes/SceneFormats/TiledMapReader.h>

#include <Engine/IO/MemoryStream.h>
#include <Engine/Bytecode/BytecodeObjectManager.h>
#include <Engine/Bytecode/BytecodeObject.h>
#include <Engine/Diagnostics/Log.h>
#include <Engine/Diagnostics/Memory.h>
#include <Engine/Hashing/FNV1A.h>
#include <Engine/Hashing/CombinedHash.h>
#include <Engine/IO/Compression/ZLibStream.h>
#include <Engine/IO/ResourceStream.h>
#include <Engine/Includes/HashMap.h>
#include <Engine/Scene/SceneLayer.h>
#include <Engine/Scene.h>

#include <Engine/TextFormats/XML/XMLParser.h>

#define TILE_FLIPX_MASK 0x80000000U
#define TILE_FLIPY_MASK 0x40000000U
#define TILE_COLLA_MASK 0x30000000U
#define TILE_COLLB_MASK 0x0C000000U
#define TILE_COLLC_MASK 0x03000000U
#define TILE_IDENT_MASK 0x00FFFFFFU

static const int decoding[] = {
    62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1,
    -1, -1, -2, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
    -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };
int   base64_decode_value(int8_t value_in) {
    value_in -= 43;
    if (value_in < 0 || value_in >= 80) { return -1; }
    return decoding[(int)value_in];
}
int   base64_decode_block(const char* code_in, const int length_in, char* plaintext_out) {
    const char* codechar = code_in;
    int8_t* plainchar = (int8_t*)plaintext_out;
    int8_t fragment;

    int  _step = 0;
    int8_t _plainchar = 0;

    *plainchar = _plainchar;

    switch (_step) {
        while (1) {
            case 0:
                do {
                    if (codechar >= code_in + length_in) {
                        _step = 0;
                        _plainchar = *plainchar;
                        return plainchar - (int8_t*)plaintext_out;
                    }
                    fragment = (char)base64_decode_value(*codechar++);
                } while (fragment < 0);
                *plainchar    = (fragment & 0x03f) << 2;

            case 1:
                do {
                    if (codechar >= code_in + length_in) {
                        _step = 1;
                        _plainchar = *plainchar;
                        return plainchar - (int8_t*)plaintext_out;
                    }
                    fragment = (char)base64_decode_value(*codechar++);
                } while (fragment < 0);
                *plainchar++ |= (fragment & 0x030) >> 4;
                *plainchar    = (fragment & 0x00f) << 4;

            case 2:
                do {
                    if (codechar >= code_in + length_in) {
                        _step = 2;
                        _plainchar = *plainchar;
                        return plainchar - (int8_t*)plaintext_out;
                    }
                    fragment = (char)base64_decode_value(*codechar++);
                } while (fragment < 0);
                *plainchar++ |= (fragment & 0x03c) >> 2;
                *plainchar    = (fragment & 0x003) << 6;

            case 3:
                do {
                    if (codechar >= code_in + length_in) {
                        _step = 3;
                        _plainchar = *plainchar;
                        return plainchar - (int8_t*)plaintext_out;
                    }
                    fragment = (char)base64_decode_value(*codechar++);
                } while (fragment < 0);
                *plainchar++   |= (fragment & 0x03f);
        }
    }
    /* control should not reach here */
    return plainchar - (int8_t*)plaintext_out;
}

PUBLIC STATIC void TiledMapReader::Read(const char* sourceF, const char* parentFolder) {
    XMLNode* tileMapXML = XMLParser::ParseFromResource(sourceF);
    if (!tileMapXML) {
        Log::Print(Log::LOG_ERROR, "Could not parse from resource \"%s\"", sourceF);
        return;
    }

    // 'infinite' maps will not work

    XMLNode* map = tileMapXML->children[0];

    if (!XMLParser::MatchToken(map->attributes.Get("version"), "1.2")) {
        Log::Print(Log::LOG_ERROR, "Only supports Tiled version 1.2!");
        XMLParser::Free(tileMapXML);
        return;
    }

    if (XMLParser::MatchToken(map->attributes.Get("infinite"), "1")) {
        Log::Print(Log::LOG_ERROR, "Not compatible with infinite maps! (Map > Map Properties > Set \"Infinite\" to unchecked)");
        XMLParser::Free(tileMapXML);
        return;
    }

    Scene::EmptyTile = 0;
    Scene::TileSize = (int)XMLParser::TokenToNumber(map->attributes.Get("tilewidth"));

    int layer_index = 0;
    int layer_width = (int)XMLParser::TokenToNumber(map->attributes.Get("width"));
    int layer_height = (int)XMLParser::TokenToNumber(map->attributes.Get("height"));

    if (Scene::ObjectLists == NULL) {
        Scene::ObjectLists = new HashMap<ObjectList*>(CombinedHash::EncryptString, 4);
        Scene::ObjectRegistries = new HashMap<ObjectList*>(CombinedHash::EncryptString, 16);
    }

    // Scene::Layers.resize(1);

    for (size_t i = 0; i < Scene::Layers.size(); i++) {
        Scene::Layers[i].Dispose();
    }
    Scene::Layers.clear();

    int LastFirstGID = 1;

    for (size_t i = 0; i < map->children.size(); i++) {
        if (XMLParser::MatchToken(map->children[i]->name, "tileset")) {
            XMLNode* tileset = map->children[i];
            int firstgid = (int)XMLParser::TokenToNumber(tileset->attributes.Get("firstgid"));

            if (firstgid == 1) {
                if (LastFirstGID < firstgid)
                    LastFirstGID = firstgid;

                XMLNode* tilesetXML = NULL;
                XMLNode* tilesetNode = NULL;

                char tilesetXMLPath[256];

                if (tileset->attributes.Exists("source")) {
                    Token source = tileset->attributes.Get("source");
                    sprintf(tilesetXMLPath, "%s%.*s", parentFolder, source.Length, source.Start);

                    tilesetXML = XMLParser::ParseFromResource(tilesetXMLPath);
                    tilesetNode = tilesetXML->children[0];
                }
                else {
                    tilesetNode = tileset;
                }

                for (size_t e = 0; e < tilesetNode->children.size(); e++) {
                    if (XMLParser::MatchToken(tilesetNode->children[e]->name, "image")) {
                        Token image_source = tilesetNode->children[e]->attributes.Get("source");
                        sprintf(tilesetXMLPath, "%s%.*s", parentFolder, image_source.Length, image_source.Start);

                        if (Scene::TileSprite) {
                            Scene::TileSprite->Dispose();
                            delete Scene::TileSprite;
                        }

                        Scene::TileSprite = new ISprite();

                        int cols, rows;
                        Scene::TileSprite->Spritesheets[0] = Scene::TileSprite->AddSpriteSheet(tilesetXMLPath);
                        cols = Scene::TileSprite->Spritesheets[0]->Width / Scene::TileSize;
                        rows = Scene::TileSprite->Spritesheets[0]->Height / Scene::TileSize;

                        Scene::TileSprite->ReserveAnimationCount(1);
                        Scene::TileSprite->AddAnimation("TileSprite", 0, 0, cols * rows);
                        for (int i = 0; i < LastFirstGID; i++) {
                            Scene::TileSprite->AddFrame(0, 0, 0, 1, 1, 0, 0);
                        }
                        for (int i = 0; i < cols * rows; i++) {
                            Scene::TileSprite->AddFrame(0,
                                (i % cols) * Scene::TileSize,
                                (i / cols) * Scene::TileSize,
                                Scene::TileSize, Scene::TileSize, -Scene::TileSize / 2, -Scene::TileSize / 2);
                        }
                    }
                }

                if (tilesetXML)
                    XMLParser::Free(tilesetXML);
            }
        }
        else if (XMLParser::MatchToken(map->children[i]->name, "layer")) {
            XMLNode* layer = map->children[i];
            layer_width = (int)XMLParser::TokenToNumber(layer->attributes.Get("width"));
            layer_height = (int)XMLParser::TokenToNumber(layer->attributes.Get("height"));

            size_t layer_size_in_bytes = layer_width * layer_height * sizeof(int);

            int* tile_buffer = NULL;

            if (layer->children.size() > 0) {
                XMLNode* data = layer->children[0];
                XMLNode* data_text = data->children[0];

                int tile_buffer_len = 0;
                if (data->attributes.Exists("encoding")) {
                    if (XMLParser::MatchToken(data->attributes.Get("encoding"), "base64")) {
                        tile_buffer = (int*)Memory::Calloc(1, layer_size_in_bytes + 4); // +4 extra space to prevent base64 overflow
                        tile_buffer_len = base64_decode_block(data_text->name.Start, data_text->name.Length, (char*)tile_buffer);
                    }
                    else if (XMLParser::MatchToken(data->attributes.Get("encoding"), "csv")) {
                        Log::Print(Log::LOG_ERROR, "Unsupported tile layer format \"CSV\"!");
                        goto FREE;
                    }
                    else {
                        Log::Print(Log::LOG_ERROR, "Unsupported tile layer format!");
                        goto FREE;
                    }
                }
                else {
                    Log::Print(Log::LOG_ERROR, "Unsupported tile layer format \"XML\"!");
                    goto FREE;
                }

                if (data->attributes.Exists("compression")) {
                    if (XMLParser::MatchToken(data->attributes.Get("compression"), "zlib")) {
                        int* decomp_tile_buffer = (int*)Memory::Malloc(layer_size_in_bytes);
                        ZLibStream::Decompress(decomp_tile_buffer, layer_size_in_bytes, tile_buffer, tile_buffer_len);
                        Memory::Free(tile_buffer);

                        tile_buffer = decomp_tile_buffer;
                    }
                    else {
                        Log::Print(Log::LOG_ERROR, "Unsupported tile layer compression format!");
                        goto FREE;
                    }
                }
            }

            Token name = layer->attributes.Get("name");

            SceneLayer scenelayer(layer_width, layer_height);
            strncpy(scenelayer.Name, name.Start, name.Length);
            scenelayer.Name[name.Length] = 0;

            scenelayer.RelativeY = 0x100;
            scenelayer.ConstantY = 0x00;
            scenelayer.Flags = SceneLayer::FLAGS_COLLIDEABLE | SceneLayer::FLAGS_NO_REPEAT_X | SceneLayer::FLAGS_NO_REPEAT_Y;
            scenelayer.DrawGroup = 0;

            if (layer->attributes.Exists("visible") && XMLParser::MatchToken(layer->attributes.Get("visible"), "0")) {
                scenelayer.Visible = false;
            }

            // NOTE: This makes all tiles Full Solid by default,
            //   so that they can be altered by an object later.
            int what_u_need = TILE_FLIPX_MASK | TILE_FLIPY_MASK | TILE_IDENT_MASK;
            for (int i = 0, iSz = layer_size_in_bytes / 4; i < iSz; i++) {
                tile_buffer[i] &= what_u_need;
                tile_buffer[i] |= TILE_COLLA_MASK;
                tile_buffer[i] |= TILE_COLLB_MASK;
            }

            memcpy(scenelayer.TilesBackup, tile_buffer, layer_size_in_bytes);
            memcpy(scenelayer.Tiles, tile_buffer, layer_size_in_bytes);

            Scene::Layers.push_back(scenelayer);

            Memory::Free(tile_buffer);
            layer_index++;
        }
        else if (XMLParser::MatchToken(map->children[i]->name, "objectgroup")) {
            XMLNode* objectgroup = map->children[i];
            for (size_t o = 0; o < objectgroup->children.size(); o++) {
                XMLNode* object = objectgroup->children[o];
                Token    object_type = object->attributes.Get("name");
                Uint32   object_type_hash = CombinedHash::EncryptData(object_type.Start, object_type.Length);
                float    object_x = XMLParser::TokenToNumber(object->attributes.Get("x"));
                float    object_y = XMLParser::TokenToNumber(object->attributes.Get("y"));
                char     object_type_string[256];

                strncpy(object_type_string, object_type.Start, object_type.Length);
                object_type_string[object_type.Length] = 0;

                ObjectList* objectList;
                if (Scene::ObjectLists->Exists(object_type_hash)) {
                    objectList = Scene::ObjectLists->Get(object_type_hash);
                    objectList->SpawnFunction = (Entity*(*)())BytecodeObjectManager::GetSpawnFunction(object_type_hash, object_type_string);
                }
                else {
                    objectList = new ObjectList();
                    strcpy(objectList->ObjectName, object_type_string);
                    Scene::ObjectLists->Put(object_type_hash, objectList);
                    objectList->SpawnFunction = (Entity*(*)())BytecodeObjectManager::GetSpawnFunction(object_type_hash, object_type_string);
                }

                if (objectList->SpawnFunction) {
                    BytecodeObject* obj = (BytecodeObject*)objectList->SpawnFunction();
                    obj->X = object_x;
                    obj->Y = object_y;
                    obj->InitialX = obj->X;
                    obj->InitialY = obj->Y;
                    obj->List = objectList;
                    Scene::AddStatic(objectList, obj);

                    if (object->attributes.Exists("width") &&
                        object->attributes.Exists("height")) {
                        obj->Properties->Put("Width", INTEGER_VAL((int)XMLParser::TokenToNumber(object->attributes.Get("width"))));
                        obj->Properties->Put("Height", INTEGER_VAL((int)XMLParser::TokenToNumber(object->attributes.Get("height"))));
                    }

                    for (size_t p = 0; p < object->children.size(); p++) {
                        if (XMLParser::MatchToken(object->children[p]->name, "properties")) {
                            XMLNode* properties = object->children[p];
                            for (size_t pr = 0; pr < properties->children.size(); pr++) {
                                if (XMLParser::MatchToken(properties->children[pr]->name, "property")) {
                                    XMLNode* property = properties->children[pr];
                                    Token    property_name = property->attributes.Get("name");
                                    Token    property_type = property->attributes.Get("type");
                                    Token    property_value = property->attributes.Get("value");

                                    char*  object_attribute_name = (char*)malloc(property_name.Length + 1);
                                    memcpy(object_attribute_name, property_name.Start, property_name.Length);
                                    object_attribute_name[property_name.Length] = 0;

                                    VMValue val = NULL_VAL;
                                    if (XMLParser::MatchToken(property_type, "int")) {
                                        val = INTEGER_VAL((int)XMLParser::TokenToNumber(property_value));
                                    }
                                    else if (XMLParser::MatchToken(property_type, "float")) {
                                        val = DECIMAL_VAL(XMLParser::TokenToNumber(property_value));
                                    }
                                    else if (XMLParser::MatchToken(property_type, "bool")) {
                                        if (XMLParser::MatchToken(property_value, "false"))
                                            val = INTEGER_VAL(0);
                                        else
                                            val = INTEGER_VAL(1);
                                    }
                                    else if (XMLParser::MatchToken(property_type, "color")) {
                                        /*
                                        property_value:
                                        In "#AARRGGBB" hex format, can also be "" empty
                                        */
                                        val = OBJECT_VAL(CopyString(property_value.Start, property_value.Length));
                                    }
                                    else if (XMLParser::MatchToken(property_type, "file")) {
                                        /*
                                        property_value:
                                        just a string
                                        */
                                        val = OBJECT_VAL(CopyString(property_value.Start, property_value.Length));
                                    }
                                    else { // implied as string
                                        val = OBJECT_VAL(CopyString(property_value.Start, property_value.Length));
                                    }

                                    obj->Properties->Put(object_attribute_name, val);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (Scene::PriorityLists) {
        size_t size = Scene::PriorityPerLayer * sizeof(vector<Entity*>);
        // Scene::PriorityLists = (vector<Entity*>*)Memory::Realloc(Scene::PriorityLists, size);

        memset(Scene::PriorityLists, 0, size);
    }
    else {
        Scene::PriorityLists = (vector<Entity*>*)Memory::TrackedCalloc("Scene::PriorityLists", Scene::PriorityPerLayer, sizeof(vector<Entity*>));
    }

    FREE:
    XMLParser::Free(tileMapXML);
}
