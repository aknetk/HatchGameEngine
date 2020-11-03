#if INTERFACE
#include <Engine/Bytecode/Types.h>
#include <Engine/ResourceTypes/ISprite.h>

class StandardLibrary {
public:

};
#endif

#include <Engine/Bytecode/StandardLibrary.h>

#include <Engine/FontFace.h>
#include <Engine/Graphics.h>
#include <Engine/Scene.h>
#include <Engine/Audio/AudioManager.h>
#include <Engine/ResourceTypes/ISound.h>
#include <Engine/Bytecode/BytecodeObject.h>
#include <Engine/Bytecode/BytecodeObjectManager.h>
#include <Engine/Bytecode/Compiler.h>
#include <Engine/Filesystem/File.h>
#include <Engine/Filesystem/Directory.h>
#include <Engine/Hashing/CombinedHash.h>
#include <Engine/Hashing/CRC32.h>
#include <Engine/Hashing/FNV1A.h>
#include <Engine/IO/FileStream.h>
#include <Engine/IO/MemoryStream.h>
#include <Engine/IO/ResourceStream.h>
#include <Engine/Math/Ease.h>
#include <Engine/Math/Math.h>
#include <Engine/Network/HTTP.h>
#include <Engine/Network/WebSocketClient.h>
#include <Engine/Rendering/GL/GLRenderer.h>
#include <Engine/Rendering/GL/GLShader.h>
#include <Engine/Rendering/Software/SoftwareRenderer.h>
#include <Engine/ResourceTypes/ImageFormats/GIF.h>
#include <Engine/ResourceTypes/ResourceType.h>
#include <Engine/TextFormats/JSON/jsmn.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define CHECK_ARGCOUNT(expects) \
    if (argCount != expects) { \
        if (BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "Expected %d arguments but got %d.", expects, argCount) == ERROR_RES_CONTINUE) \
            return NULL_VAL; \
        return NULL_VAL; \
    }
#define CHECK_AT_LEAST_ARGCOUNT(expects) \
    if (argCount < expects) { \
        if (BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "Expected at least %d arguments but got %d.", expects, argCount) == ERROR_RES_CONTINUE) \
            return NULL_VAL; \
        return NULL_VAL; \
    }

// Get([0-9A-Za-z]+)\(([0-9A-Za-z]+), ([0-9A-Za-z]+)\)
// Get$1($2, $3, threadID)

namespace LOCAL {
    inline int             GetInteger(VMValue* args, int index, Uint32 threadID) {
        int value = 0;
        switch (args[index].Type) {
            case VAL_INTEGER:
            case VAL_LINKED_INTEGER:
                value = AS_INTEGER(args[index]);
                break;
            default:
                if (BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false,
                    "Expected argument %d to be of type %s instead of %s.", index + 1, "Integer", GetTypeString(args[index])) == ERROR_RES_CONTINUE)
                    BytecodeObjectManager::Threads[threadID].ReturnFromNative();
        }
        return value;
    }
    inline float           GetDecimal(VMValue* args, int index, Uint32 threadID) {
        float value = 0.0f;
        switch (args[index].Type) {
            case VAL_DECIMAL:
            case VAL_LINKED_DECIMAL:
                value = AS_DECIMAL(args[index]);
                break;
            case VAL_INTEGER:
            case VAL_LINKED_INTEGER:
                value = AS_DECIMAL(BytecodeObjectManager::CastValueAsDecimal(args[index]));
                break;
            default:
                if (BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false,
                    "Expected argument %d to be of type %s instead of %s.", index + 1, "Decimal", GetTypeString(args[index])) == ERROR_RES_CONTINUE)
                    BytecodeObjectManager::Threads[threadID].ReturnFromNative();
        }
        return value;
    }

    inline char*           GetString(VMValue* args, int index, Uint32 threadID) {
        char* value = NULL;

        if (BytecodeObjectManager::Lock()) {
            if (!IS_STRING(args[index])) {
                if (BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false,
                    "Expected argument %d to be of type %s instead of %s.", index + 1, "String", GetTypeString(args[index])) == ERROR_RES_CONTINUE) {
                    BytecodeObjectManager::Unlock();
                    BytecodeObjectManager::Threads[threadID].ReturnFromNative();
                }
            }

            value = AS_CSTRING(args[index]);
            BytecodeObjectManager::Unlock();
        }
        return value;
    }
    inline ObjArray*       GetArray(VMValue* args, int index, Uint32 threadID) {
        ObjArray* value = NULL;
        if (BytecodeObjectManager::Lock()) {
            if (!IS_ARRAY(args[index])) {
                if (BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false,
                    "Expected argument %d to be of type %s instead of %s.", index + 1, "Array", GetTypeString(args[index])) == ERROR_RES_CONTINUE)
                    BytecodeObjectManager::Threads[threadID].ReturnFromNative();
            }

            value = (ObjArray*)(AS_OBJECT(args[index]));
            BytecodeObjectManager::Unlock();
        }
        return value;
    }
    inline ObjMap*         GetMap(VMValue* args, int index, Uint32 threadID) {
        ObjMap* value = NULL;
        if (BytecodeObjectManager::Lock()) {
            if (!IS_MAP(args[index]))
                if (BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false,
                    "Expected argument %d to be of type %s instead of %s.", index + 1, "Map", GetTypeString(args[index])) == ERROR_RES_CONTINUE)
                    BytecodeObjectManager::Threads[threadID].ReturnFromNative();

            value = (ObjMap*)(AS_OBJECT(args[index]));
            BytecodeObjectManager::Unlock();
        }
        return value;
    }
    inline ObjBoundMethod* GetBoundMethod(VMValue* args, int index, Uint32 threadID) {
        ObjBoundMethod* value = NULL;
        if (BytecodeObjectManager::Lock()) {
            if (!IS_BOUND_METHOD(args[index]))
                if (BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false,
                    "Expected argument %d to be of type %s instead of %s.", index + 1, "Event", GetTypeString(args[index])) == ERROR_RES_CONTINUE)
                    BytecodeObjectManager::Threads[threadID].ReturnFromNative();

            value = (ObjBoundMethod*)(AS_OBJECT(args[index]));
            BytecodeObjectManager::Unlock();
        }
        return value;
    }

    inline ISprite*        GetSprite(VMValue* args, int index, Uint32 threadID) {
        int where = GetInteger(args, index, threadID);
        if (where < 0 || where > (int)Scene::SpriteList.size()) {
            if (BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false,
                "Sprite index \"%d\" outside bounds of list.", where) == ERROR_RES_CONTINUE)
                BytecodeObjectManager::Threads[threadID].ReturnFromNative();
        }

        if (!Scene::SpriteList[where]) return NULL;

        return Scene::SpriteList[where]->AsSprite;
    }
    inline MediaBag*       GetVideo(VMValue* args, int index, Uint32 threadID) {
        int where = GetInteger(args, index, threadID);
        if (where < 0 || where > (int)Scene::MediaList.size()) {
            if (BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false,
                "Video index \"%d\" outside bounds of list.", where) == ERROR_RES_CONTINUE)
                BytecodeObjectManager::Threads[threadID].ReturnFromNative();
        }

        if (!Scene::MediaList[where]) return NULL;

        return Scene::MediaList[where]->AsMedia;
    }
}

// NOTE:
// Integers specifically need to be whole integers.
// Floats can be just any countable real number.
PUBLIC STATIC int       StandardLibrary::GetInteger(VMValue* args, int index) {
    Uint32 threadID = 0;
    return LOCAL::GetInteger(args, index, threadID);
}
PUBLIC STATIC float     StandardLibrary::GetDecimal(VMValue* args, int index) {
    Uint32 threadID = 0;
    return LOCAL::GetDecimal(args, index, threadID);
}
PUBLIC STATIC char*     StandardLibrary::GetString(VMValue* args, int index) {
    Uint32 threadID = 0;
    return LOCAL::GetString(args, index, threadID);
}
PUBLIC STATIC ObjArray* StandardLibrary::GetArray(VMValue* args, int index) {
    Uint32 threadID = 0;
    return LOCAL::GetArray(args, index, threadID);
}
PUBLIC STATIC ISprite*  StandardLibrary::GetSprite(VMValue* args, int index) {
    Uint32 threadID = 0;
    return LOCAL::GetSprite(args, index, threadID);
}

PUBLIC STATIC void      StandardLibrary::CheckArgCount(int argCount, int expects) {
    Uint32 threadID = 0;
    if (argCount != expects) {
        if (BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "Expected %d arguments but got %d.", expects, argCount) == ERROR_RES_CONTINUE)
            BytecodeObjectManager::Threads[threadID].ReturnFromNative();
    }
}
PUBLIC STATIC void      StandardLibrary::CheckAtLeastArgCount(int argCount, int expects) {
    Uint32 threadID = 0;
    if (argCount < expects) {
        if (BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "Expected at least %d arguments but got %d.", expects, argCount) == ERROR_RES_CONTINUE)
            BytecodeObjectManager::Threads[threadID].ReturnFromNative();
    }
}

using namespace LOCAL;

Uint8 String_ToUpperCase_Map_ExtendedASCII[0x100];
Uint8 String_ToLowerCase_Map_ExtendedASCII[0x100];

// #region Array
/***
 * Array.Create
 * \desc Creates an array.
 * \param size (Integer): Size of the array.
 * \return A reference value to the array.
 * \ns Array
 */
VMValue Array_Create(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);

    if (BytecodeObjectManager::Lock()) {
        ObjArray* array = NewArray();
        int       length = GetInteger(args, 0, threadID);
        for (int i = 0; i < length; i++) {
            array->Values->push_back(NULL_VAL);
        }

        BytecodeObjectManager::Unlock();
        return OBJECT_VAL(array);
    }
    return NULL_VAL;
}
/***
 * Array.Length
 * \desc Gets the length of an array.
 * \param array (Array): Array to get the length of.
 * \return Length of the array.
 * \ns Array
 */
VMValue Array_Length(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);

    if (BytecodeObjectManager::Lock()) {
        ObjArray* array = GetArray(args, 0, threadID);
        int size = (int)array->Values->size();
        BytecodeObjectManager::Unlock();
        return INTEGER_VAL(size);
    }
    return INTEGER_VAL(0);
}
/***
 * Array.Push
 * \desc Adds a value to the end of an array.
 * \param array (Array): Array to get the length of.
 * \param value (Value): Value to add to the array.
 * \ns Array
 */
VMValue Array_Push(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);

    if (BytecodeObjectManager::Lock()) {
        ObjArray* array = GetArray(args, 0, threadID);
        array->Values->push_back(args[1]);
        BytecodeObjectManager::Unlock();
    }
    return NULL_VAL;
}
/***
 * Array.Pop
 * \desc Gets the value at the end of an array, and removes it.
 * \param array (Array): Array to get the length of.
 * \return The value from the end of the array.
 * \ns Array
 */
VMValue Array_Pop(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);

    if (BytecodeObjectManager::Lock()) {
        ObjArray* array = GetArray(args, 0, threadID);
        VMValue   value = array->Values->back();
        array->Values->pop_back();
        BytecodeObjectManager::Unlock();
        return value;
    }
    return NULL_VAL;
}
/***
 * Array.Insert
 * \desc Inserts a value at an index of an array.
 * \param array (Array): Array to insert value.
 * \param index (Integer): Index to insert value.
 * \param value (Value): Value to insert.
 * \ns Array
 */
VMValue Array_Insert(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);

    if (BytecodeObjectManager::Lock()) {
        ObjArray* array = GetArray(args, 0, threadID);
        int       index = GetInteger(args, 1, threadID);
        array->Values->insert(array->Values->begin() + index, args[2]);
        BytecodeObjectManager::Unlock();
    }
    return NULL_VAL;
}
/***
 * Array.Erase
 * \desc Erases a value at an index of an array.
 * \param array (Array): Array to erase value.
 * \param index (Integer): Index to erase value.
 * \ns Array
 */
VMValue Array_Erase(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);

    if (BytecodeObjectManager::Lock()) {
        ObjArray* array = GetArray(args, 0, threadID);
        int       index = GetInteger(args, 1, threadID);
        array->Values->erase(array->Values->begin() + index);
        BytecodeObjectManager::Unlock();
    }
    return NULL_VAL;
}
/***
 * Array.Clear
 * \desc Clears an array.
 * \param array (Array): Array to clear.
 * \ns Array
 */
VMValue Array_Clear(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);

    if (BytecodeObjectManager::Lock()) {
        ObjArray* array = GetArray(args, 0, threadID);
        array->Values->clear();
        BytecodeObjectManager::Unlock();
    }
    return NULL_VAL;
}
/***
 * Array.Shift
 * \desc Clears an array.
 * \param array (Array): Array to clear.
 * \ns Array
 */
VMValue Array_Shift(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    if (BytecodeObjectManager::Lock()) {
        ObjArray* array = GetArray(args, 0, threadID);
        int       toright = GetInteger(args, 1, threadID);

        if (toright) {
            int lastIndex = array->Values->size() - 1;
            VMValue temp = (*array->Values)[lastIndex];
            array->Values->erase(array->Values->begin() + lastIndex);
            array->Values->insert(array->Values->begin(), temp);
        }
        else {
            VMValue temp = (*array->Values)[0];
            array->Values->erase(array->Values->begin() + 0);
            array->Values->push_back(temp);
        }

        BytecodeObjectManager::Unlock();
    }
    return NULL_VAL;
}
// #endregion

// #region Date
/***
 * Date.GetEpoch
 * \desc Gets the amount of seconds from 1 January 1970, 0:00 UTC.
 * \return The amount of seconds from epoch.
 * \ns Date
 */
VMValue Date_GetEpoch(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    return INTEGER_VAL((int)time(NULL));
}
// #endregion

// #region Device
/***
 * Device.GetPlatform
 * \desc Gets the id of the platform the application is currently running on.
 * \return ID of the current platform.
 * \ns Device
 */
VMValue Device_GetPlatform(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    return INTEGER_VAL((int)Application::Platform);
}
/***
 * Device.IsMobile
 * \desc Determines whether or not the application is running on a mobile device.
 * \return 1 if the device is on a mobile device, 0 if otherwise.
 * \ns Device
 */
VMValue Device_IsMobile(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    bool isMobile =
        Application::Platform == Platforms::iOS ||
        Application::Platform == Platforms::Android;
    return INTEGER_VAL((int)isMobile);
}
// #endregion

// #region Directory
/***
 * Directory.Create
 * \desc Creates a folder at the path.
 * \param path (String):
 * \return Returns 1 if the folder creation was successful, 0 if otherwise
 * \ns Directory
 */
VMValue Directory_Create(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    char* directory = GetString(args, 0, threadID);
    return INTEGER_VAL(Directory::Create(directory));
}
/***
 * Directory.Exists
 * \desc Determines if the folder at the path exists.
 * \param path (String):
 * \return Returns 1 if the folder exists, 0 if otherwise
 * \ns Directory
 */
VMValue Directory_Exists(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    char* directory = GetString(args, 0, threadID);
    return INTEGER_VAL(Directory::Exists(directory));
}
/***
 * Directory.GetFiles
 * \ns Directory
 */
VMValue Directory_GetFiles(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);
    ObjArray* array     = NULL;
    char*     directory = GetString(args, 0, threadID);
    char*     pattern   = GetString(args, 1, threadID);
    int       allDirs   = GetInteger(args, 2, threadID);

    vector<char*> fileList;
    // printf("finding files...\n");
    Directory::GetFiles(&fileList, directory, pattern, allDirs);
    // printf("done!\n");

    if (BytecodeObjectManager::Lock()) {
        array = NewArray();
        for (size_t i = 0; i < fileList.size(); i++) {
            ObjString* part = CopyString(fileList[i], strlen(fileList[i]));
            array->Values->push_back(OBJECT_VAL(part));
            free(fileList[i]);
        }
        BytecodeObjectManager::Unlock();
    }

    return OBJECT_VAL(array);
}
/***
 * Directory.GetDirectories
 * \ns Directory
 */
VMValue Directory_GetDirectories(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);
    ObjArray* array     = NULL;
    char*     directory = GetString(args, 0, threadID);
    char*     pattern   = GetString(args, 1, threadID);
    int       allDirs   = GetInteger(args, 2, threadID);

    vector<char*> fileList;
    Directory::GetDirectories(&fileList, directory, pattern, allDirs);

    if (BytecodeObjectManager::Lock()) {
        array = NewArray();
        for (size_t i = 0; i < fileList.size(); i++) {
            ObjString* part = CopyString(fileList[i], strlen(fileList[i]));
            array->Values->push_back(OBJECT_VAL(part));
            free(fileList[i]);
        }
        BytecodeObjectManager::Unlock();
    }

    return OBJECT_VAL(array);
}
// #endregion

// #region Display
/***
 * Display.GetWidth
 * \desc Gets the width of the current display
 * \return Returns the width of the current display
 * \ns Display
 */
VMValue Display_GetWidth(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);
    return INTEGER_VAL(dm.w);
}
/***
 * Display.GetHeight
 * \desc Gets the height of the current display
 * \return Returns the height of the current display
 * \ns Display
 */
VMValue Display_GetHeight(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);
    return INTEGER_VAL(dm.h);
}
// #endregion

// #region Draw
/***
 * Draw.Sprite
 * \desc Draws a sprite
 * \param sprite (Integer): Index of the loaded sprite.
 * \param animation (Integer): Index of the animation entry.
 * \param frame (Integer): Index of the frame in the animation entry.
 * \param x (Number): X position of where to draw the sprite.
 * \param y (Number): Y position of where to draw the sprite.
 * \param flipX (Integer): Whether or not to flip the sprite horizontally.
 * \param flipY (Integer): Whether or not to flip the sprite vertically.
 * \param scaleX (Number): Scale multiplier of the sprite horizontally.
 * \param scaleY (Number): Scale multiplier of the sprite vertically.
 * \param rotation (Number): Rotation of the drawn sprite in radians.
 * \ns Draw
 */
VMValue Draw_Sprite(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_AT_LEAST_ARGCOUNT(7);

    ISprite* sprite = Scene::SpriteList[GetInteger(args, 0, threadID)]->AsSprite;
    int animation = GetInteger(args, 1, threadID);
    int frame = GetInteger(args, 2, threadID);
    int x = (int)GetDecimal(args, 3, threadID);
    int y = (int)GetDecimal(args, 4, threadID);
    int flipX = GetInteger(args, 5, threadID);
    int flipY = GetInteger(args, 6, threadID);
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float rotation = 0.0f;
    if (argCount > 7) {
        scaleX = GetDecimal(args, 7, threadID);
        scaleY = GetDecimal(args, 8, threadID);
        rotation = GetDecimal(args, 9, threadID);
    }

    // Graphics::SetBlendColor(1.0, 1.0, 1.0, 1.0);
    Graphics::DrawSprite(sprite, animation, frame, x, y, flipX, flipY, scaleX, scaleY, rotation);
    return NULL_VAL;
}
/***
 * Draw.SpritePart
 * \desc Draws part of a sprite
 * \param sprite (Integer): Index of the loaded sprite.
 * \param animation (Integer): Index of the animation entry.
 * \param frame (Integer): Index of the frame in the animation entry.
 * \param x (Number): X position of where to draw the sprite.
 * \param y (Number): Y position of where to draw the sprite.
 * \param partX (Integer): X coordinate of part of frame to draw.
 * \param partY (Integer): Y coordinate of part of frame to draw.
 * \param partW (Integer): Width of part of frame to draw.
 * \param partH (Integer): Height of part of frame to draw.
 * \param flipX (Integer): Whether or not to flip the sprite horizontally.
 * \param flipY (Integer): Whether or not to flip the sprite vertically.
 * \param scaleX (Number): Scale multiplier of the sprite horizontally.
 * \param scaleY (Number): Scale multiplier of the sprite vertically.
 * \param rotation (Number): Rotation of the drawn sprite in radians.
 * \ns Draw
 */
VMValue Draw_SpritePart(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_AT_LEAST_ARGCOUNT(11);

    ISprite* sprite = Scene::SpriteList[GetInteger(args, 0, threadID)]->AsSprite;
    int animation = GetInteger(args, 1, threadID);
    int frame = GetInteger(args, 2, threadID);
    int x = (int)GetDecimal(args, 3, threadID);
    int y = (int)GetDecimal(args, 4, threadID);
    int sx = (int)GetDecimal(args, 5, threadID);
    int sy = (int)GetDecimal(args, 6, threadID);
    int sw = (int)GetDecimal(args, 7, threadID);
    int sh = (int)GetDecimal(args, 8, threadID);
    int flipX = GetInteger(args, 9, threadID);
    int flipY = GetInteger(args, 10, threadID);
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float rotation = 0.0f;
    if (argCount > 11) {
        scaleX = GetDecimal(args, 11, threadID);
        scaleY = GetDecimal(args, 12, threadID);
        rotation = GetDecimal(args, 13, threadID);
    }

    // Graphics::SetBlendColor(1.0, 1.0, 1.0, 1.0);
    Graphics::DrawSpritePart(sprite, animation, frame, sx, sy, sw, sh, x, y, flipX, flipY, scaleX, scaleY, rotation);
    return NULL_VAL;
}
/***
 * Draw.Image
 * \desc Draws an image
 * \param image (Integer): Index of the loaded image.
 * \param x (Number): X position of where to draw the image.
 * \param y (Number): Y position of where to draw the image.
 * \ns Draw
 */
VMValue Draw_Image(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);

	int index = GetInteger(args, 0, threadID);
	if (index < 0)
		return NULL_VAL;

    Image* image = Scene::ImageList[index]->AsImage;
    float x = GetDecimal(args, 1, threadID);
    float y = GetDecimal(args, 2, threadID);

    Graphics::DrawTexture(image->TexturePtr, 0, 0, image->TexturePtr->Width, image->TexturePtr->Height, x, y, image->TexturePtr->Width, image->TexturePtr->Height);
    return NULL_VAL;
}
/***
 * Draw.ImagePart
 * \desc Draws part of an image
 * \param image (Integer): Index of the loaded image.
 * \param partX (Integer): X coordinate of part of image to draw.
 * \param partY (Integer): Y coordinate of part of image to draw.
 * \param partW (Integer): Width of part of image to draw.
 * \param partH (Integer): Height of part of image to draw.
 * \param x (Number): X position of where to draw the image.
 * \param y (Number): Y position of where to draw the image.
 * \ns Draw
 */
VMValue Draw_ImagePart(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(7);

	int index = GetInteger(args, 0, threadID);
	if (index < 0)
		return NULL_VAL;

	Image* image = Scene::ImageList[index]->AsImage;
    float sx = GetDecimal(args, 1, threadID);
    float sy = GetDecimal(args, 2, threadID);
    float sw = GetDecimal(args, 3, threadID);
    float sh = GetDecimal(args, 4, threadID);
    float x = GetDecimal(args, 5, threadID);
    float y = GetDecimal(args, 6, threadID);

    Graphics::DrawTexture(image->TexturePtr, sx, sy, sw, sh, x, y, sw, sh);
    return NULL_VAL;
}
/***
 * Draw.ImageSized
 * \desc Draws an image, but sized
 * \param x (Number): X position of where to draw the image.
 * \param y (Number): Y position of where to draw the image.
 * \param width (Number): Width to draw the image.
 * \param height (Number): Height to draw the image.
 * \ns Draw
 */
VMValue Draw_ImageSized(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(5);

	int index = GetInteger(args, 0, threadID);
	if (index < 0)
		return NULL_VAL;

	Image* image = Scene::ImageList[index]->AsImage;
    float x = GetDecimal(args, 1, threadID);
    float y = GetDecimal(args, 2, threadID);
    float w = GetDecimal(args, 3, threadID);
    float h = GetDecimal(args, 4, threadID);

    Graphics::DrawTexture(image->TexturePtr, 0, 0, image->TexturePtr->Width, image->TexturePtr->Height, x, y, w, h);
    return NULL_VAL;
}
/***
 * Draw.ImagePartSized
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_ImagePartSized(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(9);

	int index = GetInteger(args, 0, threadID);
	if (index < 0)
		return NULL_VAL;

	Image* image = Scene::ImageList[index]->AsImage;
    float sx = GetDecimal(args, 1, threadID);
    float sy = GetDecimal(args, 2, threadID);
    float sw = GetDecimal(args, 3, threadID);
    float sh = GetDecimal(args, 4, threadID);
    float x = GetDecimal(args, 5, threadID);
    float y = GetDecimal(args, 6, threadID);
    float w = GetDecimal(args, 7, threadID);
    float h = GetDecimal(args, 8, threadID);

    Graphics::DrawTexture(image->TexturePtr, sx, sy, sw, sh, x, y, w, h);
    return NULL_VAL;
}
/***
 * Draw.Video
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_Video(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);

    MediaBag* video = GetVideo(args, 0, threadID);
    float x = GetDecimal(args, 1, threadID);
    float y = GetDecimal(args, 2, threadID);

    Graphics::DrawTexture(video->VideoTexture, 0, 0, video->VideoTexture->Width, video->VideoTexture->Height, x, y, video->VideoTexture->Width, video->VideoTexture->Height);
    return NULL_VAL;
}
/***
 * Draw.VideoPart
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_VideoPart(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(7);

    MediaBag* video = GetVideo(args, 0, threadID);
    float sx = GetDecimal(args, 1, threadID);
    float sy = GetDecimal(args, 2, threadID);
    float sw = GetDecimal(args, 3, threadID);
    float sh = GetDecimal(args, 4, threadID);
    float x = GetDecimal(args, 5, threadID);
    float y = GetDecimal(args, 6, threadID);

    Graphics::DrawTexture(video->VideoTexture, sx, sy, sw, sh, x, y, sw, sh);
    return NULL_VAL;
}
/***
 * Draw.VideoSized
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_VideoSized(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(5);

    MediaBag* video = GetVideo(args, 0, threadID);
    float x = GetDecimal(args, 1, threadID);
    float y = GetDecimal(args, 2, threadID);
    float w = GetDecimal(args, 3, threadID);
    float h = GetDecimal(args, 4, threadID);

    #ifndef NO_LIBAV
    video->Player->GetVideoData(video->VideoTexture);
    #endif

    Graphics::DrawTexture(video->VideoTexture, 0, 0, video->VideoTexture->Width, video->VideoTexture->Height, x, y, w, h);
    return NULL_VAL;
}
/***
 * Draw.VideoPartSized
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_VideoPartSized(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(9);

    MediaBag* video = GetVideo(args, 0, threadID);
    float sx = GetDecimal(args, 1, threadID);
    float sy = GetDecimal(args, 2, threadID);
    float sw = GetDecimal(args, 3, threadID);
    float sh = GetDecimal(args, 4, threadID);
    float x = GetDecimal(args, 5, threadID);
    float y = GetDecimal(args, 6, threadID);
    float w = GetDecimal(args, 7, threadID);
    float h = GetDecimal(args, 8, threadID);

    Graphics::DrawTexture(video->VideoTexture, sx, sy, sw, sh, x, y, w, h);
    return NULL_VAL;
}
/***
 * Draw.Tile
 * \param x (Number): X position of where to draw the tile.
 * \param y (Number): Y position of where to draw the tile.
 * \param flipX (Integer): Whether or not to flip the tile horizontally.
 * \param flipY (Integer): Whether or not to flip the tile vertically.
 * \ns Draw
 */
VMValue Draw_Tile(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(5);

    int id = GetInteger(args, 0, threadID);
    int x = (int)GetDecimal(args, 1, threadID) + 8;
    int y = (int)GetDecimal(args, 2, threadID) + 8;
    int flipX = GetInteger(args, 3, threadID);
    int flipY = GetInteger(args, 4, threadID);

    if (Scene::TileSprite) {
        // Graphics::SetBlendColor(1.0, 1.0, 1.0, 1.0);
        Graphics::DrawSprite(Scene::TileSprite, 0, id, x, y, flipX, flipY, 1.0f, 1.0f, 0.0f);
    }
    return NULL_VAL;
}
/***
 * Draw.Texture
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_Texture(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);

    Texture* texture = Graphics::TextureMap->Get((Uint32)GetInteger(args, 0, threadID));
    float x = GetDecimal(args, 1, threadID);
    float y = GetDecimal(args, 2, threadID);

    Graphics::DrawTexture(texture, 0, 0, texture->Width, texture->Height, x, y, texture->Width, texture->Height);
    return NULL_VAL;
}
/***
 * Draw.TextureSized
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_TextureSized(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(5);

    Texture* texture = Graphics::TextureMap->Get((Uint32)GetInteger(args, 0, threadID));
    float x = GetDecimal(args, 1, threadID);
    float y = GetDecimal(args, 2, threadID);
    float w = GetDecimal(args, 3, threadID);
    float h = GetDecimal(args, 4, threadID);

    Graphics::DrawTexture(texture, 0, 0, texture->Width, texture->Height, x, y, w, h);
    return NULL_VAL;
}
/***
 * Draw.TexturePart
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_TexturePart(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(7);

    Texture* texture = Graphics::TextureMap->Get((Uint32)GetInteger(args, 0, threadID));
    float sx = GetDecimal(args, 1, threadID);
    float sy = GetDecimal(args, 2, threadID);
    float sw = GetDecimal(args, 3, threadID);
    float sh = GetDecimal(args, 4, threadID);
    float x = GetDecimal(args, 5, threadID);
    float y = GetDecimal(args, 6, threadID);

    Graphics::DrawTexture(texture, sx, sy, sw, sh, x, y, sw, sh);
    return NULL_VAL;
}

float textAlign = 0.0f;
float textBaseline = 0.0f;
float textAscent = 1.25f;
float textAdvance = 1.0f;
int _Text_GetLetter(int l) {
    if (l < 0)
        return ' ';
    return l;
}
/***
 * Draw.SetFont
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_SetFont(int argCount, VMValue* args, Uint32 threadID) {
    return NULL_VAL;
}
/***
 * Draw.SetTextAlign
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_SetTextAlign(int argCount, VMValue* args, Uint32 threadID) { // left, center, right
    textAlign = GetInteger(args, 0, threadID) / 2.0f;
    return NULL_VAL;
}
/***
 * Draw.SetTextBaseline
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_SetTextBaseline(int argCount, VMValue* args, Uint32 threadID) { // top, baseline, bottom
    textBaseline = GetInteger(args, 0, threadID) / 2.0f;
    return NULL_VAL;
}
/***
 * Draw.SetTextAdvance
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_SetTextAdvance(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    textAdvance = GetDecimal(args, 0, threadID);
    return NULL_VAL;
}
/***
 * Draw.SetTextLineAscent
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_SetTextLineAscent(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    textAscent = GetDecimal(args, 0, threadID);
    return NULL_VAL;
}
/***
 * Draw.MeasureText
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_MeasureText(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);

    ISprite* sprite = GetSprite(args, 0, threadID);
    char*    text   = GetString(args, 1, threadID);

    float x = 0.0, y = 0.0;
    float maxW = 0.0, maxH = 0.0;
    float lineHeight = sprite->Animations[0].FrameToLoop;
    for (char* i = text; *i; i++) {
        if (*i == '\n') {
            x = 0.0;
            y += lineHeight * textAscent;
            goto __MEASURE_Y;
        }

        x += sprite->Animations[0].Frames[*i].Advance * textAdvance;

        if (maxW < x)
            maxW = x;

        __MEASURE_Y:
        if (maxH < y + (sprite->Animations[0].Frames[*i].Height - sprite->Animations[0].Frames[*i].OffsetY))
            maxH = y + (sprite->Animations[0].Frames[*i].Height - sprite->Animations[0].Frames[*i].OffsetY);
    }

    if (BytecodeObjectManager::Lock()) {
        ObjArray* array = NewArray();
        array->Values->push_back(DECIMAL_VAL(maxW));
        array->Values->push_back(DECIMAL_VAL(maxH));
        BytecodeObjectManager::Unlock();
        return OBJECT_VAL(array);
    }
    return NULL_VAL;
}
/***
 * Draw.MeasureTextWrapped
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_MeasureTextWrapped(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_AT_LEAST_ARGCOUNT(3);

    ISprite* sprite = GetSprite(args, 0, threadID);
    char*    text   = GetString(args, 1, threadID);
    float    max_w  = GetDecimal(args, 2, threadID);
    int      maxLines = 0x7FFFFFFF;
    if (argCount > 3)
        maxLines = GetInteger(args, 3, threadID);

    int word = 0;
    char* linestart = text;
    char* wordstart = text;

    float x = 0.0, y = 0.0;
    float maxW = 0.0, maxH = 0.0;
    float lineHeight = sprite->Animations[0].FrameToLoop;

    int lineNo = 1;
    for (char* i = text; ; i++) {
        if (((*i == ' ' || *i == 0) && i != wordstart) || *i == '\n') {
            float testWidth = 0.0f;
            for (char* o = linestart; o < i; o++) {
                testWidth += sprite->Animations[0].Frames[*o].Advance * textAdvance;
            }
            if ((testWidth > max_w && word > 0) || *i == '\n') {
                x = 0.0f;
                for (char* o = linestart; o < wordstart - 1; o++) {
                    x += sprite->Animations[0].Frames[*o].Advance * textAdvance;

                    if (maxW < x)
                        maxW = x;
                    if (maxH < y + (sprite->Animations[0].Frames[*o].Height + sprite->Animations[0].Frames[*o].OffsetY))
                        maxH = y + (sprite->Animations[0].Frames[*o].Height + sprite->Animations[0].Frames[*o].OffsetY);
                }

                if (lineNo == maxLines)
                    goto FINISH;
                lineNo++;

                linestart = wordstart;
                y += lineHeight * textAscent;
            }

            wordstart = i + 1;
            word++;
        }

        if (!*i)
            break;
    }

    x = 0.0f;
    for (char* o = linestart; *o; o++) {
        x += sprite->Animations[0].Frames[*o].Advance * textAdvance;
        if (maxW < x)
            maxW = x;
        if (maxH < y + (sprite->Animations[0].Frames[*o].Height + sprite->Animations[0].Frames[*o].OffsetY))
            maxH = y + (sprite->Animations[0].Frames[*o].Height + sprite->Animations[0].Frames[*o].OffsetY);
    }

    FINISH:
    if (BytecodeObjectManager::Lock()) {
        ObjArray* array = NewArray();
        array->Values->push_back(DECIMAL_VAL(maxW));
        array->Values->push_back(DECIMAL_VAL(maxH));
        BytecodeObjectManager::Unlock();
        return OBJECT_VAL(array);
    }
    return NULL_VAL;
}
/***
 * Draw.Text
 * \param sprite (Integer): Index of the loaded sprite to be used as text.
 * \param text (String): Text to draw.
 * \param x (Number): X position of where to draw the tile.
 * \param y (Number): Y position of where to draw the tile.
 * \ns Draw
 */
VMValue Draw_Text(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(4);

    ISprite* sprite = GetSprite(args, 0, threadID);
    char*    text   = GetString(args, 1, threadID);
    float    basex = GetDecimal(args, 2, threadID);
    float    basey = GetDecimal(args, 3, threadID);

    float    x = basex;
    float    y = basey;
    float*   lineWidths;
    int      line = 0;

    // Count lines
    for (char* i = text; *i; i++) {
        if (*i == '\n') {
            line++;
            continue;
        }
    }
    line++;
    lineWidths = (float*)malloc(line * sizeof(float));
    if (!lineWidths)
        return NULL_VAL;

    // Get line widths
    line = 0;
    x = 0.0f;
    for (char* i = text, l; *i; i++) {
        l = _Text_GetLetter((Uint8)*i);
        if (l == '\n') {
            lineWidths[line++] = x;
            x = 0.0f;
            continue;
        }
        x += sprite->Animations[0].Frames[l].Advance * textAdvance;
    }
    lineWidths[line++] = x;

    // Draw text
    line = 0;
    x = basex;
    bool lineBack = true;
    for (char* i = text, l; *i; i++) {
        l = _Text_GetLetter((Uint8)*i);
        if (lineBack) {
            x -= sprite->Animations[0].Frames[l].OffsetX;
            lineBack = false;
        }

        if (l == '\n') {
            x = basex;
            y += sprite->Animations[0].FrameToLoop * textAscent;
            lineBack = true;
            line++;
            continue;
        }

        Graphics::DrawSprite(sprite, 0, l, x - lineWidths[line] * textAlign, y - sprite->Animations[0].AnimationSpeed * textBaseline, false, false, 1.0f, 1.0f, 0.0f);
        x += sprite->Animations[0].Frames[l].Advance * textAdvance;
    }

    free(lineWidths);
    return NULL_VAL;
}
/***
 * Draw.TextWrapped
 * \param sprite (Integer): Index of the loaded sprite to be used as text.
 * \param text (String): Text to draw.
 * \param x (Number): X position of where to draw the tile.
 * \param y (Number): Y position of where to draw the tile.
 * \param maxWidth (Number): Max width the text can draw in.
 * \param maxLines (Integer): Max lines the text can draw.
 * \ns Draw
 */
VMValue Draw_TextWrapped(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_AT_LEAST_ARGCOUNT(5);

    ISprite* sprite = GetSprite(args, 0, threadID);
    char*    text   = GetString(args, 1, threadID);
    float    basex = GetDecimal(args, 2, threadID);
    float    basey = GetDecimal(args, 3, threadID);
    float    max_w = GetDecimal(args, 4, threadID);
    int      maxLines = 0x7FFFFFFF;
    if (argCount > 5)
        maxLines = GetInteger(args, 5, threadID);

    float    x = basex;
    float    y = basey;

    // Draw text
    int   word = 0;
    char* linestart = text;
    char* wordstart = text;
    bool  lineBack = true;
    int   lineNo = 1;
    for (char* i = text, l; ; i++) {
        l = _Text_GetLetter((Uint8)*i);
        if (((l == ' ' || l == 0) && i != wordstart) || l == '\n') {
            float testWidth = 0.0f;
            for (char* o = linestart, lm; o < i; o++) {
                lm = _Text_GetLetter((Uint8)*o);
                testWidth += sprite->Animations[0].Frames[lm].Advance * textAdvance;
            }

            if ((testWidth > max_w && word > 0) || l == '\n') {
                float lineWidth = 0.0f;
                for (char* o = linestart, lm; o < wordstart - 1; o++) {
                    lm = _Text_GetLetter((Uint8)*o);
                    if (lineBack) {
                        lineWidth -= sprite->Animations[0].Frames[lm].OffsetX;
                        lineBack = false;
                    }
                    lineWidth += sprite->Animations[0].Frames[lm].Advance * textAdvance;
                }
                lineBack = true;

                x = basex - lineWidth * textAlign;
                for (char* o = linestart, lm; o < wordstart - 1; o++) {
                    lm = _Text_GetLetter((Uint8)*o);
                    if (lineBack) {
                        x -= sprite->Animations[0].Frames[lm].OffsetX;
                        lineBack = false;
                    }
                    Graphics::DrawSprite(sprite, 0, lm, x, y - sprite->Animations[0].AnimationSpeed * textBaseline, false, false, 1.0f, 1.0f, 0.0f);
                    x += sprite->Animations[0].Frames[lm].Advance * textAdvance;
                }

                if (lineNo == maxLines)
                    return NULL_VAL;

                lineNo++;

                linestart = wordstart;
                y += sprite->Animations[0].FrameToLoop * textAscent;
                lineBack = true;
            }

            wordstart = i + 1;
            word++;
        }
        if (!l)
            break;
    }

    float lineWidth = 0.0f;
    for (char* o = linestart, l; *o; o++) {
        l = _Text_GetLetter((Uint8)*o);
        if (lineBack) {
            lineWidth -= sprite->Animations[0].Frames[l].OffsetX;
            lineBack = false;
        }
        lineWidth += sprite->Animations[0].Frames[l].Advance * textAdvance;
    }
    lineBack = true;

    x = basex - lineWidth * textAlign;
    for (char* o = linestart, l; *o; o++) {
        l = _Text_GetLetter((Uint8)*o);
        if (lineBack) {
            x -= sprite->Animations[0].Frames[l].OffsetX;
            lineBack = false;
        }
        Graphics::DrawSprite(sprite, 0, l, x, y - sprite->Animations[0].AnimationSpeed * textBaseline, false, false, 1.0f, 1.0f, 0.0f);
        x += sprite->Animations[0].Frames[l].Advance * textAdvance;
    }

    // FINISH:

    return NULL_VAL;
}
/***
 * Draw.TextEllipsis
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_TextEllipsis(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(5);

    ISprite* sprite = Scene::SpriteList[GetInteger(args, 0, threadID)]->AsSprite;
    char*    text = GetString(args, 1, threadID);
    float    x = GetDecimal(args, 2, threadID);
    float    y = GetDecimal(args, 3, threadID);
    float    maxwidth = GetDecimal(args, 4, threadID);

    float    elpisswidth = sprite->Animations[0].Frames['.'].Advance * 3;

    int t;
    size_t textlen = strlen(text);
    float textwidth = 0.0f;
    for (size_t i = 0; i < textlen; i++) {
        t = (int)text[i];
        textwidth += sprite->Animations[0].Frames[t].Advance;
    }
    // If smaller than or equal to maxwidth, just draw normally.
    if (textwidth <= maxwidth) {
        for (size_t i = 0; i < textlen; i++) {
            t = (int)text[i];
            Graphics::DrawSprite(sprite, 0, t, x, y, false, false, 1.0f, 1.0f, 0.0f);
            x += sprite->Animations[0].Frames[t].Advance;
        }
    }
    else {
        for (size_t i = 0; i < textlen; i++) {
            t = (int)text[i];
            if (x + sprite->Animations[0].Frames[t].Advance + elpisswidth > maxwidth) {
                Graphics::DrawSprite(sprite, 0, '.', x, y, false, false, 1.0f, 1.0f, 0.0f);
                x += sprite->Animations[0].Frames['.'].Advance;
                Graphics::DrawSprite(sprite, 0, '.', x, y, false, false, 1.0f, 1.0f, 0.0f);
                x += sprite->Animations[0].Frames['.'].Advance;
                Graphics::DrawSprite(sprite, 0, '.', x, y, false, false, 1.0f, 1.0f, 0.0f);
                x += sprite->Animations[0].Frames['.'].Advance;
                break;
            }
            Graphics::DrawSprite(sprite, 0, t, x, y, false, false, 1.0f, 1.0f, 0.0f);
            x += sprite->Animations[0].Frames[t].Advance;
        }
    }
    // Graphics::DrawSprite(sprite, 0, t, x, y, false, false, 1.0f, 1.0f, 0.0f);
    return NULL_VAL;
}

/***
 * Draw.SetBlendColor
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_SetBlendColor(int argCount, VMValue* args, Uint32 threadID) {
    if (argCount <= 2) {
        CHECK_ARGCOUNT(2);
        int hex = GetInteger(args, 0, threadID);
        float alpha = GetDecimal(args, 1, threadID);
        Graphics::SetBlendColor(
            (hex >> 16 & 0xFF) / 255.f,
            (hex >> 8 & 0xFF) / 255.f,
            (hex & 0xFF) / 255.f, alpha);
        return NULL_VAL;
    }
    CHECK_ARGCOUNT(4);
    Graphics::SetBlendColor(GetDecimal(args, 0, threadID), GetDecimal(args, 1, threadID), GetDecimal(args, 2, threadID), GetDecimal(args, 3, threadID));
    return NULL_VAL;
}
/***
 * Draw.SetTextureBlend
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_SetTextureBlend(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    Graphics::TextureBlend = !!GetInteger(args, 0, threadID);
    return NULL_VAL;
}
/***
 * Draw.SetBlendMode
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_SetBlendMode(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    switch (Graphics::BlendMode = GetInteger(args, 0, threadID)) {
        case BlendMode_NORMAL:
            Graphics::SetBlendMode(
                BlendFactor_SRC_ALPHA, BlendFactor_INV_SRC_ALPHA,
                BlendFactor_SRC_ALPHA, BlendFactor_INV_SRC_ALPHA);
            break;
        case BlendMode_ADD:
            Graphics::SetBlendMode(
                BlendFactor_SRC_ALPHA, BlendFactor_ONE,
                BlendFactor_SRC_ALPHA, BlendFactor_ONE);
            break;
        case BlendMode_MAX:
            Graphics::SetBlendMode(
                BlendFactor_SRC_ALPHA, BlendFactor_INV_SRC_COLOR,
                BlendFactor_SRC_ALPHA, BlendFactor_INV_SRC_COLOR);
            break;
        case BlendMode_SUBTRACT:
            Graphics::SetBlendMode(
                BlendFactor_ZERO, BlendFactor_INV_SRC_COLOR,
                BlendFactor_SRC_ALPHA, BlendFactor_INV_SRC_ALPHA);
            break;
        default:
        Graphics::SetBlendMode(
            BlendFactor_SRC_ALPHA, BlendFactor_INV_SRC_ALPHA,
            BlendFactor_SRC_ALPHA, BlendFactor_INV_SRC_ALPHA);
    }
    return NULL_VAL;
}
/***
 * Draw.SetBlendFactor
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_SetBlendFactor(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    int src = GetInteger(args, 0, threadID);
    int dst = GetInteger(args, 1, threadID);
    Graphics::SetBlendMode(src, dst, src, dst);
    return NULL_VAL;
}
/***
 * Draw.SetBlendFactorExtended
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_SetBlendFactorExtended(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(4);
    int srcC = GetInteger(args, 0, threadID);
    int dstC = GetInteger(args, 1, threadID);
    int srcA = GetInteger(args, 2, threadID);
    int dstA = GetInteger(args, 3, threadID);
    Graphics::SetBlendMode(srcC, dstC, srcA, dstA);
    return NULL_VAL;
}
/***
 * Draw.SetCompareColor
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_SetCompareColor(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int hex = GetInteger(args, 0, threadID);
    SoftwareRenderer::CompareColor = 0xFF000000U | hex;
    return NULL_VAL;
}

/***
 * Draw.Line
 * \param x1 (Number): X position of where to start drawing the line.
 * \param y1 (Number): Y position of where to start drawing the line.
 * \param x2 (Number): X position of where to end drawing the line.
 * \param y2 (Number): Y position of where to end drawing the line.
 * \ns Draw
 */
VMValue Draw_Line(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(4);
    Graphics::StrokeLine(GetDecimal(args, 0, threadID), GetDecimal(args, 1, threadID), GetDecimal(args, 2, threadID), GetDecimal(args, 3, threadID));
    return NULL_VAL;
}
/***
 * Draw.Circle
 * \param x (Number): Center X position of where to draw the cricle.
 * \param y (Number): Center Y position of where to draw the cricle.
 * \param radius (Number): Radius of the circle.
 * \ns Draw
 */
VMValue Draw_Circle(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);
    Graphics::FillCircle(GetDecimal(args, 0, threadID), GetDecimal(args, 1, threadID), GetDecimal(args, 2, threadID));
    return NULL_VAL;
}
/***
 * Draw.Ellipse
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_Ellipse(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(4);
    Graphics::FillEllipse(GetDecimal(args, 0, threadID), GetDecimal(args, 1, threadID), GetDecimal(args, 2, threadID), GetDecimal(args, 3, threadID));
    return NULL_VAL;
}
/***
 * Draw.Triangle
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_Triangle(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(6);
    Graphics::FillTriangle(
        GetDecimal(args, 0, threadID), GetDecimal(args, 1, threadID),
        GetDecimal(args, 2, threadID), GetDecimal(args, 3, threadID),
        GetDecimal(args, 4, threadID), GetDecimal(args, 5, threadID));
    return NULL_VAL;
}
/***
 * Draw.Rectangle
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_Rectangle(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(4);
    Graphics::FillRectangle(GetDecimal(args, 0, threadID), GetDecimal(args, 1, threadID), GetDecimal(args, 2, threadID), GetDecimal(args, 3, threadID));
    return NULL_VAL;
}
/***
 * Draw.CircleStroke
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_CircleStroke(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);
    Graphics::StrokeCircle(GetDecimal(args, 0, threadID), GetDecimal(args, 1, threadID), GetDecimal(args, 2, threadID));
    return NULL_VAL;
}
/***
 * Draw.EllipseStroke
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_EllipseStroke(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(4);
    Graphics::StrokeEllipse(GetDecimal(args, 0, threadID), GetDecimal(args, 1, threadID), GetDecimal(args, 2, threadID), GetDecimal(args, 3, threadID));
    return NULL_VAL;
}
/***
 * Draw.TriangleStroke
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_TriangleStroke(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(6);
    Graphics::StrokeTriangle(
        GetDecimal(args, 0, threadID), GetDecimal(args, 1, threadID),
        GetDecimal(args, 2, threadID), GetDecimal(args, 3, threadID),
        GetDecimal(args, 4, threadID), GetDecimal(args, 5, threadID));
    return NULL_VAL;
}
/***
 * Draw.RectangleStroke
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_RectangleStroke(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(4);
    Graphics::StrokeRectangle(GetDecimal(args, 0, threadID), GetDecimal(args, 1, threadID), GetDecimal(args, 2, threadID), GetDecimal(args, 3, threadID));
    return NULL_VAL;
}
/***
 * Draw.UseFillSmoothing
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_UseFillSmoothing(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    Graphics::SmoothFill = !!GetInteger(args, 0, threadID);
    return NULL_VAL;
}
/***
 * Draw.UseStrokeSmoothing
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_UseStrokeSmoothing(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    Graphics::SmoothStroke = !!GetInteger(args, 0, threadID);
    return NULL_VAL;
}

/***
 * Draw.SetClip
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_SetClip(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(4);
    Graphics::SetClip((int)GetDecimal(args, 0, threadID), (int)GetDecimal(args, 1, threadID), (int)GetDecimal(args, 2, threadID), (int)GetDecimal(args, 3, threadID));
    return NULL_VAL;
}
/***
 * Draw.ClearClip
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_ClearClip(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    Graphics::ClearClip();
    return NULL_VAL;
}

/***
 * Draw.Save
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_Save(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    Graphics::Save();
    return NULL_VAL;
}
/***
 * Draw.Scale
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_Scale(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_AT_LEAST_ARGCOUNT(2);
    float x = GetDecimal(args, 0, threadID);
    float y = GetDecimal(args, 1, threadID);
    float z = 1.0f; if (argCount == 3) z = GetDecimal(args, 2, threadID);
    Graphics::Scale(x, y, z);
    return NULL_VAL;
}
/***
 * Draw.Rotate
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_Rotate(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_AT_LEAST_ARGCOUNT(1);
    if (argCount == 3) {
        float x = GetDecimal(args, 0, threadID);
        float y = GetDecimal(args, 1, threadID);
        float z = GetDecimal(args, 2, threadID);
        Graphics::Rotate(x, y, z);
    }
    else {
        float z = GetDecimal(args, 0, threadID);
        Graphics::Rotate(0.0f, 0.0f, z);
    }
    return NULL_VAL;
}
/***
 * Draw.Restore
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_Restore(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    Graphics::Restore();
    return NULL_VAL;
}
/***
 * Draw.Translate
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_Translate(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_AT_LEAST_ARGCOUNT(2);
    float x = GetDecimal(args, 0, threadID);
    float y = GetDecimal(args, 1, threadID);
    float z = 0.0f; if (argCount == 3) z = GetDecimal(args, 2, threadID);
    Graphics::Translate(x, y, z);
    return NULL_VAL;
}

/***
 * Draw.SetTextureTarget
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_SetTextureTarget(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);

    Texture* texture = Graphics::TextureMap->Get((Uint32)GetInteger(args, 0, threadID));
    Graphics::SetRenderTarget(texture);
    return NULL_VAL;
}
/***
 * Draw.Clear
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_Clear(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    Graphics::Clear();
    return NULL_VAL;
}
/***
 * Draw.ResetTextureTarget
 * \desc
 * \return
 * \ns Draw
 */
VMValue Draw_ResetTextureTarget(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    Graphics::SetRenderTarget(!Scene::Views[Scene::ViewCurrent].UseDrawTarget ? NULL : Scene::Views[Scene::ViewCurrent].DrawTarget);
    // Graphics::UpdateOrtho(Scene::Views[Scene::ViewCurrent].Width, Scene::Views[Scene::ViewCurrent].Height);

    // View* currentView = &Scene::Views[Scene::ViewCurrent];
    // Graphics::UpdatePerspective(45.0, currentView->Width / currentView->Height, 0.1, 1000.0);
    // currentView->Z = 500.0;
    // Matrix4x4::Scale(currentView->ProjectionMatrix, currentView->ProjectionMatrix, 1.0, -1.0, 1.0);
    // Matrix4x4::Translate(currentView->ProjectionMatrix, currentView->ProjectionMatrix, -currentView->X - currentView->Width / 2, -currentView->Y - currentView->Height / 2, -currentView->Z);
	Graphics::UpdateProjectionMatrix();
    return NULL_VAL;
}
// #endregion

// #region Ease
/***
 * Ease.InSine
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_InSine(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::InSine(GetDecimal(args, 0, threadID))); }
/***
 * Ease.OutSine
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_OutSine(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::OutSine(GetDecimal(args, 0, threadID))); }
/***
 * Ease.InOutSine
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_InOutSine(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::InOutSine(GetDecimal(args, 0, threadID))); }
/***
 * Ease.InQuad
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_InQuad(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::InQuad(GetDecimal(args, 0, threadID))); }
/***
 * Ease.OutQuad
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_OutQuad(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::OutQuad(GetDecimal(args, 0, threadID))); }
/***
 * Ease.InOutQuad
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_InOutQuad(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::InOutQuad(GetDecimal(args, 0, threadID))); }
/***
 * Ease.InCubic
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_InCubic(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::InCubic(GetDecimal(args, 0, threadID))); }
/***
 * Ease.OutCubic
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_OutCubic(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::OutCubic(GetDecimal(args, 0, threadID))); }
/***
 * Ease.InOutCubic
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_InOutCubic(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::InOutCubic(GetDecimal(args, 0, threadID))); }
/***
 * Ease.InQuart
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_InQuart(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::InQuart(GetDecimal(args, 0, threadID))); }
/***
 * Ease.OutQuart
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_OutQuart(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::OutQuart(GetDecimal(args, 0, threadID))); }
/***
 * Ease.InOutQuart
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_InOutQuart(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::InOutQuart(GetDecimal(args, 0, threadID))); }
/***
 * Ease.InQuint
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_InQuint(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::InQuint(GetDecimal(args, 0, threadID))); }
/***
 * Ease.OutQuint
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_OutQuint(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::OutQuint(GetDecimal(args, 0, threadID))); }
/***
 * Ease.InOutQuint
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_InOutQuint(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::InOutQuint(GetDecimal(args, 0, threadID))); }
/***
 * Ease.InExpo
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_InExpo(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::InExpo(GetDecimal(args, 0, threadID))); }
/***
 * Ease.OutExpo
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_OutExpo(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::OutExpo(GetDecimal(args, 0, threadID))); }
/***
 * Ease.InOutExpo
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_InOutExpo(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::InOutExpo(GetDecimal(args, 0, threadID))); }
/***
 * Ease.InCirc
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_InCirc(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::InCirc(GetDecimal(args, 0, threadID))); }
/***
 * Ease.OutCirc
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_OutCirc(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::OutCirc(GetDecimal(args, 0, threadID))); }
/***
 * Ease.InOutCirc
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_InOutCirc(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::InOutCirc(GetDecimal(args, 0, threadID))); }
/***
 * Ease.InBack
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_InBack(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::InBack(GetDecimal(args, 0, threadID))); }
/***
 * Ease.OutBack
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_OutBack(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::OutBack(GetDecimal(args, 0, threadID))); }
/***
 * Ease.InOutBack
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_InOutBack(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::InOutBack(GetDecimal(args, 0, threadID))); }
/***
 * Ease.InElastic
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_InElastic(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::InElastic(GetDecimal(args, 0, threadID))); }
/***
 * Ease.OutElastic
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_OutElastic(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::OutElastic(GetDecimal(args, 0, threadID))); }
/***
 * Ease.InOutElastic
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_InOutElastic(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::InOutElastic(GetDecimal(args, 0, threadID))); }
/***
 * Ease.InBounce
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_InBounce(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::InBounce(GetDecimal(args, 0, threadID))); }
/***
 * Ease.OutBounce
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_OutBounce(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::OutBounce(GetDecimal(args, 0, threadID))); }
/***
 * Ease.InOutBounce
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_InOutBounce(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::InOutBounce(GetDecimal(args, 0, threadID))); }
/***
 * Ease.Triangle
 * \desc
 * \return
 * \ns Ease
 */
VMValue Ease_Triangle(int argCount, VMValue* args, Uint32 threadID) { CHECK_ARGCOUNT(1); return NUMBER_VAL(Ease::Triangle(GetDecimal(args, 0, threadID))); }
// #endregion

// #region File
/***
 * File.Exists
 * \desc
 * \return
 * \ns File
 */
VMValue File_Exists(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    char* filePath = GetString(args, 0, threadID);
    return INTEGER_VAL(File::Exists(filePath));
}
/***
 * File.ReadAllText
 * \desc
 * \return
 * \ns File
 */
VMValue File_ReadAllText(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    char* filePath = GetString(args, 0, threadID);
    FileStream* stream = FileStream::New(filePath, FileStream::READ_ACCESS);
    if (!stream)
        return NULL_VAL;

    if (BytecodeObjectManager::Lock()) {
        size_t size = stream->Length();
        ObjString* text = AllocString(size);
        stream->ReadBytes(text->Chars, size);
        stream->Close();
        BytecodeObjectManager::Unlock();
        return OBJECT_VAL(text);
    }
    return NULL_VAL;
}
/***
 * File.WriteAllText
 * \desc
 * \return
 * \ns File
 */
VMValue File_WriteAllText(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    char* filePath = GetString(args, 0, threadID);
    // To verify 2nd argument is string
    GetString(args, 1, threadID);
    if (BytecodeObjectManager::Lock()) {
        ObjString* text = AS_STRING(args[1]);

        FileStream* stream = FileStream::New(filePath, FileStream::WRITE_ACCESS);
        if (!stream) {
            BytecodeObjectManager::Unlock();
            return INTEGER_VAL(false);
        }

        stream->WriteBytes(text->Chars, text->Length);
        stream->Close();

        BytecodeObjectManager::Unlock();
        return INTEGER_VAL(true);
    }
    return NULL_VAL;
}
// #endregion

// #region HTTP
struct _HTTP_Bundle {
    char* url;
    char* filename;
    ObjBoundMethod callback;
};
int _HTTP_GetToFile(void* opaque) {
    _HTTP_Bundle* bundle = (_HTTP_Bundle*)opaque;

    size_t length;
    Uint8* data = NULL;
    if (HTTP::GET(bundle->url, &data, &length, NULL)) {
        FileStream* stream = FileStream::New(bundle->filename, FileStream::WRITE_ACCESS);
        if (stream) {
            stream->WriteBytes(data, length);
            stream->Close();
        }
        Memory::Free(data);
    }
    free(bundle);
    return 0;
}

/***
 * HTTP.GetString
 * \desc
 * \return
 * \ns HTTP
 */
VMValue HTTP_GetString(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    char*           url = NULL;
    ObjBoundMethod* callback = NULL;

    url = GetString(args, 0, threadID);
    if (IS_BOUND_METHOD(args[1])) {
        callback = GetBoundMethod(args, 1, threadID);
    }

    size_t length;
    Uint8* data = NULL;
    if (!HTTP::GET(url, &data, &length, callback)) {
        return NULL_VAL;
    }

    VMValue obj = NULL_VAL;
    if (BytecodeObjectManager::Lock()) {
        obj = OBJECT_VAL(TakeString((char*)data, length));
        BytecodeObjectManager::Unlock();
    }
    return obj;
}
/***
 * HTTP.GetToFile
 * \desc
 * \return
 * \ns HTTP
 */
VMValue HTTP_GetToFile(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);
    char* url = GetString(args, 0, threadID);
    char* filename = GetString(args, 1, threadID);
    bool  blocking = GetInteger(args, 2, threadID);

    size_t url_sz = strlen(url) + 1;
    size_t filename_sz = strlen(filename) + 1;
    _HTTP_Bundle* bundle = (_HTTP_Bundle*)malloc(sizeof(_HTTP_Bundle) + url_sz + filename_sz);
    bundle->url = (char*)(bundle + 1);
    bundle->filename = bundle->url + url_sz;
    strcpy(bundle->url, url);
    strcpy(bundle->filename, filename);

    if (blocking) {
        _HTTP_GetToFile(bundle);
    }
    else {
        SDL_CreateThread(_HTTP_GetToFile, "HTTP.GetToFile", bundle);
    }
    return NULL_VAL;
}
// #endregion

// #region Input
/***
 * Input.GetMouseX
 * \desc
 * \return
 * \ns Input
 */
VMValue Input_GetMouseX(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    int value; SDL_GetMouseState(&value, NULL);
    return NUMBER_VAL((float)value);
}
/***
 * Input.GetMouseY
 * \desc
 * \return
 * \ns Input
 */
VMValue Input_GetMouseY(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    int value; SDL_GetMouseState(NULL, &value);
    return NUMBER_VAL((float)value);
}
/***
 * Input.IsMouseButtonDown
 * \desc
 * \return
 * \ns Input
 */
VMValue Input_IsMouseButtonDown(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int button = GetInteger(args, 0, threadID);
    return INTEGER_VAL((InputManager::MouseDown >> button) & 1);
}
/***
 * Input.IsMouseButtonPressed
 * \desc
 * \return
 * \ns Input
 */
VMValue Input_IsMouseButtonPressed(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int button = GetInteger(args, 0, threadID);
    return INTEGER_VAL((InputManager::MousePressed >> button) & 1);
}
/***
 * Input.IsMouseButtonReleased
 * \desc
 * \return
 * \ns Input
 */
VMValue Input_IsMouseButtonReleased(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int button = GetInteger(args, 0, threadID);
    return INTEGER_VAL((InputManager::MouseReleased >> button) & 1);
}
/***
 * Input.IsKeyDown
 * \desc
 * \return
 * \ns Input
 */
VMValue Input_IsKeyDown(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int key = GetInteger(args, 0, threadID);
    return INTEGER_VAL(InputManager::KeyboardState[key]);
}
/***
 * Input.IsKeyPressed
 * \desc
 * \return
 * \ns Input
 */
VMValue Input_IsKeyPressed(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int key = GetInteger(args, 0, threadID);
    int down = InputManager::KeyboardState[key] && !InputManager::KeyboardStateLast[key];
    return INTEGER_VAL(down);
}
/***
 * Input.IsKeyReleased
 * \desc
 * \return
 * \ns Input
 */
VMValue Input_IsKeyReleased(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int key = GetInteger(args, 0, threadID);
    int down = !InputManager::KeyboardState[key] && InputManager::KeyboardStateLast[key];
    return INTEGER_VAL(down);
}
/***
 * Input.GetControllerAttached
 * \desc
 * \return
 * \ns Input
 */
VMValue Input_GetControllerAttached(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int controller_index = GetInteger(args, 0, threadID);
    return INTEGER_VAL(InputManager::GetControllerAttached(controller_index));
}
/***
 * Input.GetControllerHat
 * \desc
 * \return
 * \ns Input
 */
VMValue Input_GetControllerHat(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    int controller_index = GetInteger(args, 0, threadID);
    int index = GetInteger(args, 1, threadID);
    return INTEGER_VAL(InputManager::GetControllerHat(controller_index, index));
}
/***
 * Input.GetControllerAxis
 * \desc
 * \return
 * \ns Input
 */
VMValue Input_GetControllerAxis(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    int controller_index = GetInteger(args, 0, threadID);
    int index = GetInteger(args, 1, threadID);
    return DECIMAL_VAL(InputManager::GetControllerAxis(controller_index, index));
}
/***
 * Input.GetControllerButton
 * \desc
 * \return
 * \ns Input
 */
VMValue Input_GetControllerButton(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    int controller_index = GetInteger(args, 0, threadID);
    int index = GetInteger(args, 1, threadID);
    return INTEGER_VAL(InputManager::GetControllerButton(controller_index, index));
}
/***
 * Input.GetControllerName
 * \desc
 * \return
 * \ns Input
 */
VMValue Input_GetControllerName(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int controller_index = GetInteger(args, 0, threadID);

    char* name = InputManager::GetControllerName(controller_index);

    if (BytecodeObjectManager::Lock()) {
        ObjString* string = CopyString(name, strlen(name));
        BytecodeObjectManager::Unlock();
        return OBJECT_VAL(string);
    }
    return NULL_VAL;
}
// #endregion

// #region IO
// #endregion

// #region Instance
/***
 * Instance.Create
 * \desc
 * \return
 * \ns Instance
 */
VMValue Instance_Create(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_AT_LEAST_ARGCOUNT(3);

    char* objectName = GetString(args, 0, threadID);
    float x = GetDecimal(args, 1, threadID);
    float y = GetDecimal(args, 2, threadID);
    int flag = argCount == 4 ? GetInteger(args, 3, threadID) : 0;

    ObjectList* objectList = NULL;
    if (!Scene::ObjectLists->Exists(objectName)) {
        objectList = new ObjectList();
        strcpy(objectList->ObjectName, objectName);
        objectList->SpawnFunction = (Entity* (*)())BytecodeObjectManager::GetSpawnFunction(CombinedHash::EncryptString(objectName), objectName);
        Scene::ObjectLists->Put(objectName, objectList);
    }
    else {
        objectList = Scene::ObjectLists->Get(objectName);
        objectList->SpawnFunction = (Entity* (*)())BytecodeObjectManager::GetSpawnFunction(CombinedHash::EncryptString(objectName), objectName);
    }

    if (!objectList->SpawnFunction) {
        BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "Object \"%s\" does not exist.", objectName);
        return NULL_VAL;
    }

    BytecodeObject* obj = (BytecodeObject*)objectList->SpawnFunction();
    obj->X = x;
    obj->Y = y;
    obj->InitialX = x;
    obj->InitialY = y;
    obj->List = objectList;
    Scene::AddDynamic(objectList, obj);

    ObjInstance* instance = obj->Instance;

    obj->Create(flag);


    return OBJECT_VAL(instance);
}
/***
 * Instance.GetNth
 * \desc
 * \return
 * \ns Instance
 */
VMValue Instance_GetNth(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);

    char* objectName = GetString(args, 0, threadID);
    int n = GetInteger(args, 1, threadID);

    if (!Scene::ObjectLists->Exists(objectName)) {
        return NULL_VAL;
    }

    ObjectList* objectList = Scene::ObjectLists->Get(objectName);
    BytecodeObject* object = (BytecodeObject*)objectList->GetNth(n);

    if (object) {
        return OBJECT_VAL(object->Instance);
    }

    return NULL_VAL;
}
/***
 * Instance.GetCount
 * \desc
 * \return
 * \ns Instance
 */
VMValue Instance_GetCount(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);

    char* objectName = GetString(args, 0, threadID);

    if (!Scene::ObjectLists->Exists(objectName)) {
        return INTEGER_VAL(0);
    }

    ObjectList* objectList = Scene::ObjectLists->Get(objectName);
    return INTEGER_VAL(objectList->Count());
}
/***
 * Instance.GetNextInstance
 * \desc
 * \return
 * \ns Instance
 */
VMValue Instance_GetNextInstance(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);

    Entity* self = (Entity*)AS_INSTANCE(args[0])->EntityPtr;
    int     n    = GetInteger(args, 1, threadID);

    Entity* object = self;
    for (int i = 0; i <= n; i++) {
        object = object->NextEntity;
        if (!object)
            return NULL_VAL;
    }

    if (object)
        return OBJECT_VAL(((BytecodeObject*)object)->Instance);

    return NULL_VAL;
}
// #endregion

// #region JSON
static int _JSON_FillMap(ObjMap*, const char*, jsmntok_t*, size_t);
static int _JSON_FillArray(ObjArray*, const char*, jsmntok_t*, size_t);

static int _JSON_FillMap(ObjMap* map, const char* text, jsmntok_t* t, size_t count) {
    jsmntok_t* key;
    jsmntok_t* value;
    if (count == 0) {
        return 0;
    }

    Uint32 keyHash;
    int tokcount = 0;
    for (int i = 0; i < t->size; i++) {
        key = t + 1 + tokcount;
        keyHash = map->Keys->HashFunction(text + key->start, key->end - key->start);
        map->Keys->Put(keyHash, HeapCopyString(text + key->start, key->end - key->start));
        tokcount += 1;
        if (key->size > 0) {
            VMValue val = NULL_VAL;
            value = t + 1 + tokcount;
            switch (value->type) {
                case JSMN_PRIMITIVE:
                    tokcount += 1;
                    if (memcmp("true", text + value->start, value->end - value->start) == 0)
                        val = INTEGER_VAL(true);
                    else if (memcmp("false", text + value->start, value->end - value->start) == 0)
                        val = INTEGER_VAL(false);
                    else if (memcmp("null", text + value->start, value->end - value->start) == 0)
                        val = NULL_VAL;
                    else {
                        bool isNumeric = true;
                        bool hasDot = false;
                        for (const char* cStart = text + value->start, *c = cStart; c < text + value->end; c++) {
                            isNumeric &= (c == cStart && *cStart == '-') || (*c >= '0' && *c <= '9') || (isNumeric && *c == '.' && c > text + value->start && !hasDot);
                            hasDot |= (*c == '.');
                        }
                        if (isNumeric) {
                            if (hasDot) {
                                val = DECIMAL_VAL((float)strtod(text + value->start, NULL));
                            }
                            else {
                                val = INTEGER_VAL((int)strtol(text + value->start, NULL, 10));
                            }
                        }
                        else
                            val = OBJECT_VAL(CopyString(text + value->start, value->end - value->start));
                    }
                    break;
                case JSMN_STRING: {
                    tokcount += 1;
                    val = OBJECT_VAL(CopyString(text + value->start, value->end - value->start));

                    char* o = AS_CSTRING(val);
                    for (const char* l = text + value->start; l < text + value->end; l++) {
                        if (*l == '\\') {
                            l++;
                            switch (*l) {
                                case '\"':
                                    *o++ = '\"'; break;
                                case '/':
                                    *o++ = '/'; break;
                                case '\\':
                                    *o++ = '\\'; break;
                                case 'b':
                                    *o++ = '\b'; break;
                                case 'f':
                                    *o++ = '\f'; break;
                                case 'r': // *o++ = '\r';
                                    break;
                                case 'n':
                                    *o++ = '\n'; break;
                                case 't':
                                    *o++ = '\t'; break;
                                case 'u':
                                    l++;
                                    l++;
                                    l++;
                                    l++;
                                    *o++ = '\t'; break;
                            }
                        }
                        else
                            *o++ = *l;
                    }
                    *o = 0;
                    break;
                }
                // /*
                case JSMN_OBJECT: {
                    ObjMap* subMap = NewMap();
                    tokcount += _JSON_FillMap(subMap, text, value, count - tokcount);
                    val = OBJECT_VAL(subMap);
                    break;
                }
                //*/
                case JSMN_ARRAY: {
                    ObjArray* subArray = NewArray();
                    tokcount += _JSON_FillArray(subArray, text, value, count - tokcount);
                    val = OBJECT_VAL(subArray);
                    break;
                }
                default:
                    break;
            }
            map->Values->Put(keyHash, val);
        }
    }
    return tokcount + 1;
}
static int _JSON_FillArray(ObjArray* arr, const char* text, jsmntok_t* t, size_t count) {
    jsmntok_t* value;
    if (count == 0) {
        return 0;
    }

    int tokcount = 0;
    for (int i = 0; i < t->size; i++) {
        VMValue val = NULL_VAL;
        value = t + 1 + tokcount;
        switch (value->type) {
            // /*
            case JSMN_PRIMITIVE:
                tokcount += 1;
                if (memcmp("true", text + value->start, value->end - value->start) == 0)
                    val = INTEGER_VAL(true);
                else if (memcmp("false", text + value->start, value->end - value->start) == 0)
                    val = INTEGER_VAL(false);
                else if (memcmp("null", text + value->start, value->end - value->start) == 0)
                    val = NULL_VAL;
                else {
                    bool isNumeric = true;
                    bool hasDot = false;
                    for (const char* cStart = text + value->start, *c = cStart; c < text + value->end; c++) {
                        isNumeric &= (c == cStart && *cStart == '-') || (*c >= '0' && *c <= '9') || (isNumeric && *c == '.' && c > text + value->start && !hasDot);
                        hasDot |= (*c == '.');
                    }
                    if (isNumeric) {
                        if (hasDot) {
                            val = DECIMAL_VAL((float)strtod(text + value->start, NULL));
                        }
                        else {
                            val = INTEGER_VAL((int)strtol(text + value->start, NULL, 10));
                        }
                    }
                    else
                        val = OBJECT_VAL(CopyString(text + value->start, value->end - value->start));
                }
                break;
            case JSMN_STRING: {
                tokcount += 1;
                val = OBJECT_VAL(CopyString(text + value->start, value->end - value->start));

                char* o = AS_CSTRING(val);
                for (const char* l = text + value->start; l < text + value->end; l++) {
                    if (*l == '\\') {
                        l++;
                        switch (*l) {
                            case '\"':
                                *o++ = '\"'; break;
                            case '/':
                                *o++ = '/'; break;
                            case '\\':
                                *o++ = '\\'; break;
                            case 'b':
                                *o++ = '\b'; break;
                            case 'f':
                                *o++ = '\f'; break;
                            case 'r': // *o++ = '\r';
                                break;
                            case 'n':
                                *o++ = '\n'; break;
                            case 't':
                                *o++ = '\t'; break;
                            case 'u':
                                l++;
                                l++;
                                l++;
                                l++;
                                *o++ = '\t'; break;
                        }
                    }
                    else
                        *o++ = *l;
                }
                *o = 0;
                break;
            }
            case JSMN_OBJECT: {
                ObjMap* subMap = NewMap();
                tokcount += _JSON_FillMap(subMap, text, value, count - tokcount);
                val = OBJECT_VAL(subMap);
                break;
            }
            case JSMN_ARRAY: {
                ObjArray* subArray = NewArray();
                tokcount += _JSON_FillArray(subArray, text, value, count - tokcount);
                val = OBJECT_VAL(subArray);
                break;
            }
            default:
                break;
        }
        arr->Values->push_back(val);
    }
    return tokcount + 1;
}

/***
 * JSON.Parse
 * \desc
 * \return
 * \ns JSON
 */
VMValue JSON_Parse(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    if (BytecodeObjectManager::Lock()) {
        ObjString* string = AS_STRING(args[0]);
        ObjMap*    map = NewMap();

        jsmn_parser p;
        jsmntok_t* tok;
        size_t tokcount = 16;
        tok = (jsmntok_t*)malloc(sizeof(*tok) * tokcount);
        if (tok == NULL) {
            return NULL_VAL;
        }

        jsmn_init(&p);
        while (true) {
            int r = jsmn_parse(&p, string->Chars, string->Length, tok, tokcount);
            if (r < 0) {
                if (r == JSMN_ERROR_NOMEM) {
                    tokcount = tokcount * 2;
                    tok = (jsmntok_t*)realloc(tok, sizeof(*tok) * tokcount);
                    if (tok == NULL) {
                        BytecodeObjectManager::Unlock();
                        return NULL_VAL;
                    }
                    continue;
                }
            }
            else {
                _JSON_FillMap(map, string->Chars, tok, p.toknext);
            }
            break;
        }
        free(tok);

        BytecodeObjectManager::Unlock();
        return OBJECT_VAL(map);
    }
    return NULL_VAL;
}
/***
 * JSON.ToString
 * \desc
 * \return
 * \ns JSON
 */
VMValue JSON_ToString(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    Compiler::PrettyPrint = !!GetInteger(args, 1, threadID);
    VMValue value = BytecodeObjectManager::CastValueAsString(args[0]);
    Compiler::PrettyPrint = true;
    return value;
}
// #endregion

// #region Math
/***
 * Math.Cos
 * \desc Returns the cosine of an angle of x radians.
 * \param x (Decimal): Angle (in radians) to get the cosine of.
 * \return The cosine of x radians.
 * \ns Math
 */
VMValue Math_Cos(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    return DECIMAL_VAL(Math::Cos(GetDecimal(args, 0, threadID)));
}
/***
 * Math.Sin
 * \desc Returns the sine of an angle of x radians.
 * \param x (Decimal): Angle (in radians) to get the sine of.
 * \return The sine of x radians.
 * \ns Math
 */
VMValue Math_Sin(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    return DECIMAL_VAL(Math::Sin(GetDecimal(args, 0, threadID)));
}
/***
 * Math.Acos
 * \desc
 * \return
 * \ns Math
 */
VMValue Math_Acos(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    return DECIMAL_VAL(Math::Acos(GetDecimal(args, 0, threadID)));
}
/***
 * Math.Asin
 * \desc
 * \return
 * \ns Math
 */
VMValue Math_Asin(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    return DECIMAL_VAL(Math::Asin(GetDecimal(args, 0, threadID)));
}
/***
 * Math.Atan
 * \desc Returns the angle (in radians) from x and y.
 * \param x (Decimal): x value.
 * \param y (Decimal): y value.
 * \return The angle from x and y.
 * \ns Math
 */
VMValue Math_Atan(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    return DECIMAL_VAL(Math::Atan(GetDecimal(args, 0, threadID), GetDecimal(args, 1, threadID)));
}
/***
 * Math.Distance
 * \desc
 * \return
 * \ns Math
 */
VMValue Math_Distance(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(4);
    return DECIMAL_VAL(Math::Distance(GetDecimal(args, 0, threadID), GetDecimal(args, 1, threadID), GetDecimal(args, 2, threadID), GetDecimal(args, 3, threadID)));
}
/***
 * Math.Direction
 * \desc
 * \return
 * \ns Math
 */
VMValue Math_Direction(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(4);
    return DECIMAL_VAL(Math::Atan(GetDecimal(args, 2, threadID) - GetDecimal(args, 0, threadID), GetDecimal(args, 1, threadID) - GetDecimal(args, 3, threadID)));
}
/***
 * Math.Abs
 * \desc
 * \return
 * \ns Math
 */
VMValue Math_Abs(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    return DECIMAL_VAL(Math::Abs(GetDecimal(args, 0, threadID)));
}
/***
 * Math.Min
 * \desc
 * \return
 * \ns Math
 */
VMValue Math_Min(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    // respect the type of number
    return DECIMAL_VAL(Math::Min(GetDecimal(args, 0, threadID), GetDecimal(args, 1, threadID)));
}
/***
 * Math.Max
 * \desc
 * \return
 * \ns Math
 */
VMValue Math_Max(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    return DECIMAL_VAL(Math::Max(GetDecimal(args, 0, threadID), GetDecimal(args, 1, threadID)));
}
/***
 * Math.Clamp
 * \desc
 * \return
 * \ns Math
 */
VMValue Math_Clamp(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);
    return DECIMAL_VAL(Math::Clamp(GetDecimal(args, 0, threadID), GetDecimal(args, 1, threadID), GetDecimal(args, 2, threadID)));
}
/***
 * Math.Sign
 * \desc
 * \return
 * \ns Math
 */
VMValue Math_Sign(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    return DECIMAL_VAL(Math::Sign(GetDecimal(args, 0, threadID)));
}
/***
 * Math.Random
 * \desc
 * \return
 * \ns Math
 */
VMValue Math_Random(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    return DECIMAL_VAL(Math::Random());
}
/***
 * Math.RandomMax
 * \desc
 * \return
 * \ns Math
 */
VMValue Math_RandomMax(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    return DECIMAL_VAL(Math::RandomMax(GetDecimal(args, 0, threadID)));
}
/***
 * Math.RandomRange
 * \desc
 * \return
 * \ns Math
 */
VMValue Math_RandomRange(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    return DECIMAL_VAL(Math::RandomRange(GetDecimal(args, 0, threadID), GetDecimal(args, 1, threadID)));
}
/***
 * Math.Floor
 * \desc
 * \return
 * \ns Math
 */
VMValue Math_Floor(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    return DECIMAL_VAL(std::floor(GetDecimal(args, 0, threadID)));
}
/***
 * Math.Ceil
 * \desc
 * \return
 * \ns Math
 */
VMValue Math_Ceil(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    return DECIMAL_VAL(std::ceil(GetDecimal(args, 0, threadID)));
}
/***
 * Math.Round
 * \desc
 * \return
 * \ns Math
 */
VMValue Math_Round(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    return DECIMAL_VAL(std::round(GetDecimal(args, 0, threadID)));
}
/***
 * Math.Sqrt
 * \desc
 * \return
 * \ns Math
 */
VMValue Math_Sqrt(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    return DECIMAL_VAL(sqrt(GetDecimal(args, 0, threadID)));
}
/***
 * Math.Pow
 * \desc
 * \return
 * \ns Math
 */
VMValue Math_Pow(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    return DECIMAL_VAL(pow(GetDecimal(args, 0, threadID), GetDecimal(args, 1, threadID)));
}
/***
 * Math.Exp
 * \desc
 * \return
 * \ns Math
 */
VMValue Math_Exp(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    return DECIMAL_VAL(std::exp(GetDecimal(args, 0, threadID)));
}
// #endregion

// #region Music
/***
 * Music.Play
 * \desc
 * \return
 * \ns Music
 */
VMValue Music_Play(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);

    ISound* audio = Scene::MusicList[GetInteger(args, 0, threadID)]->AsMusic;

    AudioManager::PushMusic(audio, false, 0);
    return NULL_VAL;
}
/***
 * Music.Stop
 * \desc
 * \return
 * \ns Music
 */
VMValue Music_Stop(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    ISound* audio = Scene::MusicList[GetInteger(args, 0, threadID)]->AsMusic;
    AudioManager::RemoveMusic(audio);
    return NULL_VAL;
}
/***
 * Music.StopWithFadeOut
 * \desc
 * \return
 * \ns Music
 */
VMValue Music_StopWithFadeOut(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    // ISound* audio = Scene::MusicList[GetInteger(args, 0, threadID)]->AsMusic;
    float seconds = GetDecimal(args, 0, threadID);
    AudioManager::FadeMusic(seconds);
    return NULL_VAL;
}
/***
 * Music.Pause
 * \desc
 * \return
 * \ns Music
 */
VMValue Music_Pause(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    AudioManager::Lock();
    if (AudioManager::MusicStack.size() > 0)
        AudioManager::MusicStack[0]->Paused = true;
    AudioManager::Unlock();
    return NULL_VAL;
}
/***
 * Music.Resume
 * \desc
 * \return
 * \ns Music
 */
VMValue Music_Resume(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    AudioManager::Lock();
    if (AudioManager::MusicStack.size() > 0)
        AudioManager::MusicStack[0]->Paused = false;
    AudioManager::Unlock();
    return NULL_VAL;
}
/***
 * Music.Clear
 * \desc
 * \return
 * \ns Music
 */
VMValue Music_Clear(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    AudioManager::ClearMusic();
    return NULL_VAL;
}
/***
 * Music.Loop
 * \desc
 * \return
 * \ns Music
 */
VMValue Music_Loop(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);
    ISound* audio = Scene::MusicList[GetInteger(args, 0, threadID)]->AsMusic;
    // int loop = GetInteger(args, 1, threadID);
    int loop_point = GetInteger(args, 2, threadID);

    AudioManager::PushMusic(audio, true, loop_point);
    return NULL_VAL;
}
/***
 * Music.IsPlaying
 * \desc
 * \return
 * \ns Music
 */
VMValue Music_IsPlaying(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    ISound* audio = Scene::MusicList[GetInteger(args, 0, threadID)]->AsMusic;
    return INTEGER_VAL(AudioManager::IsPlayingMusic(audio));
}
// #endregion

// #region Number
/***
 * Number.ToString
 * \desc
 * \return
 * \ns Number
 */
VMValue Number_ToString(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_AT_LEAST_ARGCOUNT(1);

    int base = 10;
    if (argCount == 2) {
        base = GetInteger(args, 1, threadID);
    }

    switch (args[0].Type) {
        case VAL_DECIMAL:
        case VAL_LINKED_DECIMAL: {
            float n = GetDecimal(args, 0, threadID);
            char temp[16];
            sprintf(temp, "%f", n);

            VMValue strng = NULL_VAL;
            if (BytecodeObjectManager::Lock()) {
                strng = OBJECT_VAL(CopyString(temp, strlen(temp)));
                BytecodeObjectManager::Unlock();
            }
            return strng;
        }
        case VAL_INTEGER:
        case VAL_LINKED_INTEGER: {
            int n = GetInteger(args, 0, threadID);
            char temp[16];
            if (base == 16)
                sprintf(temp, "0x%X", n);
            else
                sprintf(temp, "%d", n);

            VMValue strng = NULL_VAL;
            if (BytecodeObjectManager::Lock()) {
                strng = OBJECT_VAL(CopyString(temp, strlen(temp)));
                BytecodeObjectManager::Unlock();
            }
            return strng;
        }
        default:
            BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false,
                "Expected argument %d to be of type %s instead of %s.", 0 + 1, "Number", GetTypeString(args[0]));
    }

    return NULL_VAL;
}
/***
 * Number.AsInteger
 * \desc
 * \return
 * \ns Number
 */
VMValue Number_AsInteger(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    return INTEGER_VAL((int)GetDecimal(args, 0, threadID));
}
/***
 * Number.AsDecimal
 * \desc
 * \return
 * \ns Number
 */
VMValue Number_AsDecimal(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    return DECIMAL_VAL(GetDecimal(args, 0, threadID));
}
// #endregion

// #region Palette
/***
 * Palette.EnablePaletteUsage
 * \desc Sets a color on the specified palette, format 0xAARRGGBB.
 * \ns Palette
 */
VMValue Palette_EnablePaletteUsage(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int usePalettes = GetInteger(args, 0, threadID);
    Graphics::UsePalettes = usePalettes;
    return NULL_VAL;
}
// .hpal defines color lines that it can load instead of full 256 color .act's
/***
 * Palette.LoadFromFile
 * \desc Loads palette from an .act or a .hpal file.
 * \param x (Decimal): Angle (in radians) to get the cosine of.
 * \ns Palette
 */
VMValue Palette_LoadFromFile(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    int palIndex = GetInteger(args, 0, threadID);
    char* filename = GetString(args, 1, threadID);
    return NULL_VAL;
}
/***
 * Palette.LoadFromResource
 * \desc Loads palette from an .act or a .hpal resource.
 * \ns Palette
 */
VMValue Palette_LoadFromResource(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    int palIndex = GetInteger(args, 0, threadID);
    char* filename = GetString(args, 1, threadID);

    ResourceStream* reader;
    if ((reader = ResourceStream::New(filename))) {
        MemoryStream* memoryReader;
        if ((memoryReader = MemoryStream::New(reader))) {
            // ACT file
            if (strstr(filename, ".act") || strstr(filename, ".ACT")) {
                do {
                    Uint8 Color[3];
                    for (int d = 0; d < 256; d++) {
                        memoryReader->ReadBytes(Color, 3);
                        SoftwareRenderer::PaletteColors[palIndex][d] = 0xFF000000U | Color[0] << 16 | Color[1] << 8 | Color[2];
                    }
                    SoftwareRenderer::ConvertFromARGBtoNative(&SoftwareRenderer::PaletteColors[palIndex][0], 256);
                } while (false);
            }
            // HPAL file
            else if (strstr(filename, ".hpal") || strstr(filename, ".HPAL")) {
                do {
                    Uint32 magic = memoryReader->ReadUInt32();
                    if (magic != 0x4C415048)
                        break;

                    Uint8 Color[3];
                    int paletteCount = memoryReader->ReadUInt32();

                    if (paletteCount > 32 - palIndex)
                        paletteCount = 32 - palIndex;

                    for (int i = 0; i < paletteCount; i++) {
                        // Palette Set
                        int bitmap = memoryReader->ReadUInt16();
                        for (int col = 0; col < 16; col++) {
                            if ((bitmap & (1 << col)) != 0) {
                                for (int d = 0; d < 16; d++) {
                                    memoryReader->ReadBytes(Color, 3);
                                    SoftwareRenderer::PaletteColors[i][(col << 4) | d] = 0xFF000000U | Color[0] << 16 | Color[1] << 8 | Color[2];
                                }
                                SoftwareRenderer::ConvertFromARGBtoNative(&SoftwareRenderer::PaletteColors[i][(col << 4)], 16);
                            }
                        }
                    }
                } while (false);
            }
            // GIF file
            else if (strstr(filename, ".gif") || strstr(filename, ".GIF")) {
                bool loadPalette = Graphics::UsePalettes;

                GIF* gif;

                Graphics::UsePalettes = false;
                gif = GIF::Load(filename);
                Graphics::UsePalettes = loadPalette;

                if (gif) {
                    if (gif->Colors) {
                        for (int p = 0; p < 256; p++)
                            SoftwareRenderer::PaletteColors[palIndex][p] = gif->Colors[p];
                        Memory::Free(gif->Colors);
                    }
                    delete gif;
                }
            }
            // RSDK StageConfig
            else if (strstr(filename, "StageConfig.bin")) {
                do {
                    Uint32 magic = memoryReader->ReadUInt32();
                    if (magic != 0x474643)
                        break;

                    memoryReader->ReadByte(); // useGameObjects

                    int objectNameCount = memoryReader->ReadByte();
                    for (int i = 0; i < objectNameCount; i++)
                        memoryReader->ReadHeaderedString();

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
                } while (false);
            }
            // RSDK GameConfig
            else if (strstr(filename, "GameConfig.bin")) {

            }

            memoryReader->Close();
        }
        reader->Close();
    }

    return NULL_VAL;
}
/***
 * Palette.GetColor
 * \desc Gets a color from the specified palette.
 * \ns Palette
 */
VMValue Palette_GetColor(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    int palIndex = GetInteger(args, 0, threadID);
    int colorIndex = GetInteger(args, 1, threadID);
    return INTEGER_VAL((int)(SoftwareRenderer::PaletteColors[palIndex][colorIndex] & 0xFF00FFU));
}
/***
 * Palette.SetColor
 * \desc Sets a color on the specified palette, format 0xAARRGGBB.
 * \ns Palette
 */
VMValue Palette_SetColor(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);
    int palIndex = GetInteger(args, 0, threadID);
    int colorIndex = GetInteger(args, 1, threadID);
    Uint32 hex = (Uint32)GetInteger(args, 2, threadID);
    SoftwareRenderer::PaletteColors[palIndex][colorIndex] = (hex & 0xFF00FFU) | 0xFF000000U;
    return NULL_VAL;
}
/***
 * Palette.MixPalettes
 * \desc Sets a color on the specified palette, format 0xAARRGGBB.
 * \ns Palette
 */
inline Uint32 PMP_ColorBlend(Uint32 color1, Uint32 color2, int percent) {
    Uint32 rb = color1 & 0xFF00FFU;
    Uint32 g  = color1 & 0x00FF00U;
    rb += (((color2 & 0xFF00FFU) - rb) * percent) >> 8;
    g  += (((color2 & 0x00FF00U) - g) * percent) >> 8;
    return (rb & 0xFF00FFU) | (g & 0x00FF00U);
}
VMValue Palette_MixPalettes(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(6);
    int palIndexDest = GetInteger(args, 0, threadID);
    int palIndex1 = GetInteger(args, 1, threadID);
    int palIndex2 = GetInteger(args, 2, threadID);
    float mix = GetDecimal(args, 3, threadID);
    int colorIndexStart = GetInteger(args, 4, threadID);
    int colorCount = GetInteger(args, 5, threadID);

    int percent = mix * 0x100;
    for (int c = colorIndexStart; c < colorIndexStart + colorCount; c++) {
        SoftwareRenderer::PaletteColors[palIndexDest][c] = 0xFF000000U | PMP_ColorBlend(SoftwareRenderer::PaletteColors[palIndex1][c], SoftwareRenderer::PaletteColors[palIndex2][c], percent);
    }
    return NULL_VAL;
}
/***
 * Palette.RotateColorsLeft
 * \desc Sets a color on the specified palette, format 0xAARRGGBB.
 * \ns Palette
 */
VMValue Palette_RotateColorsLeft(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);
    int palIndex = GetInteger(args, 0, threadID);
    int colorIndexStart = GetInteger(args, 1, threadID);
    int count = GetInteger(args, 2, threadID);

    if (count > 0x100 - colorIndexStart)
        count = 0x100 - colorIndexStart;

    Uint32 temp = SoftwareRenderer::PaletteColors[palIndex][colorIndexStart];
    for (int i = colorIndexStart + 1; i < colorIndexStart + count; i++) {
        SoftwareRenderer::PaletteColors[palIndex][i - 1] = SoftwareRenderer::PaletteColors[palIndex][i];
    }
    SoftwareRenderer::PaletteColors[palIndex][colorIndexStart + count - 1] = temp;
    return NULL_VAL;
}
/***
 * Palette.RotateColorsRight
 * \desc Sets a color on the specified palette, format 0xAARRGGBB.
 * \ns Palette
 */
VMValue Palette_RotateColorsRight(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);
    int palIndex = GetInteger(args, 0, threadID);
    int colorIndexStart = GetInteger(args, 1, threadID);
    int count = GetInteger(args, 2, threadID);

    if (count > 0x100 - colorIndexStart)
        count = 0x100 - colorIndexStart;

    Uint32 temp = SoftwareRenderer::PaletteColors[palIndex][colorIndexStart + count - 1];
    for (int i = colorIndexStart + count - 1; i >= colorIndexStart; i--) {
        SoftwareRenderer::PaletteColors[palIndex][i] = SoftwareRenderer::PaletteColors[palIndex][i - 1];
    }
    SoftwareRenderer::PaletteColors[palIndex][colorIndexStart] = temp;
    return NULL_VAL;
}
/***
 * Palette.CopyColors
 * \desc Sets a color on the specified palette, format 0xAARRGGBB.
 * \ns Palette
 */
VMValue Palette_CopyColors(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(5);
    int palIndexFrom = GetInteger(args, 0, threadID);
    int colorIndexStartFrom = GetInteger(args, 1, threadID);
    int palIndexTo = GetInteger(args, 2, threadID);
    int colorIndexStartTo = GetInteger(args, 3, threadID);
    int count = GetInteger(args, 4, threadID);

    if (count > 0x100 - colorIndexStartTo)
        count = 0x100 - colorIndexStartTo;
    if (count > 0x100 - colorIndexStartFrom)
        count = 0x100 - colorIndexStartFrom;

    memcpy(&SoftwareRenderer::PaletteColors[palIndexTo][colorIndexStartTo], &SoftwareRenderer::PaletteColors[palIndexFrom][colorIndexStartFrom], count * sizeof(Uint32));
    return NULL_VAL;
}
/***
 * Palette.SetPaletteIndexLines
 * \desc Sets a color on the specified palette, format 0xAARRGGBB.
 * \ns Palette
 */
VMValue Palette_SetPaletteIndexLines(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);
    int palIndex = GetInteger(args, 0, threadID);
    int lineStart = GetInteger(args, 1, threadID);
    int lineEnd = GetInteger(args, 2, threadID);

    if (lineStart < 0)
        lineStart = 0;

    for (int i = lineStart; i < lineEnd && i < sizeof(SoftwareRenderer::PaletteIndexLines); i++) {
        SoftwareRenderer::PaletteIndexLines[i] = (Uint8)palIndex;
    }
    return NULL_VAL;
}
// #endregion

// #region Resources
// return true if we found it in the list
bool    GetResourceListSpace(vector<ResourceType*>* list, ResourceType* resource, size_t* index, bool* foundEmpty) {
    *foundEmpty = false;
    *index = list->size();
    for (size_t i = 0, listSz = list->size(); i < listSz; i++) {
        if (!(*list)[i]) {
            if (!(*foundEmpty)) {
                *foundEmpty = true;
                *index = i;
            }
            continue;
        }
        if ((*list)[i]->FilenameHash == resource->FilenameHash) {
            *index = i;
            return true;
        }
    }
    return false;
}
/***
 * Resources.LoadSprite
 * \desc
 * \return
 * \ns Resources
 */
VMValue Resources_LoadSprite(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    char*  filename = GetString(args, 0, threadID);

    ResourceType* resource = new ResourceType;
    resource->FilenameHash = CRC32::EncryptString(filename);
    resource->UnloadPolicy = GetInteger(args, 1, threadID);

    // size_t listSz = Scene::SpriteList.size();
    // for (size_t i = 0; i < listSz; i++)
    //     if (Scene::SpriteList[i]->FilenameHash == resource->FilenameHash)
    //         return INTEGER_VAL((int)i);
    size_t index = 0;
    bool emptySlot = false;
    vector<ResourceType*>* list = &Scene::SpriteList;
    if (GetResourceListSpace(list, resource, &index, &emptySlot))
        return INTEGER_VAL((int)index);
    else if (emptySlot) (*list)[index] = resource; else list->push_back(resource);

    resource->AsSprite = new ISprite(filename);
    return INTEGER_VAL((int)index);
}
/***
 * Resources.LoadImage
 * \desc
 * \return
 * \ns Resources
 */
VMValue Resources_LoadImage(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    char*  filename = GetString(args, 0, threadID);

    ResourceType* resource = new ResourceType;
    resource->FilenameHash = CRC32::EncryptString(filename);
    resource->UnloadPolicy = GetInteger(args, 1, threadID);

    size_t index = 0;
    bool emptySlot = false;
    vector<ResourceType*>* list = &Scene::ImageList;
    if (GetResourceListSpace(list, resource, &index, &emptySlot))
        return INTEGER_VAL((int)index);
    else if (emptySlot) (*list)[index] = resource; else list->push_back(resource);

    resource->AsImage = new Image(filename);
    if (!resource->AsImage->TexturePtr) {
        delete resource->AsImage;
        delete resource;
        (*list)[index] = NULL;
        return INTEGER_VAL(-1);
    }
    return INTEGER_VAL((int)index);
}
/***
 * Resources.LoadFont
 * \desc
 * \return
 * \ns Resources
 */
VMValue Resources_LoadFont(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);
    char*  filename = GetString(args, 0, threadID);
    int    pixel_sz = (int)GetDecimal(args, 1, threadID);

    ResourceType* resource = new ResourceType;
    resource->FilenameHash = CRC32::EncryptString(filename);
    resource->FilenameHash = CRC32::EncryptData(&pixel_sz, sizeof(int), resource->FilenameHash);
    resource->UnloadPolicy = GetInteger(args, 2, threadID);

    size_t index = 0;
    bool emptySlot = false;
    vector<ResourceType*>* list = &Scene::SpriteList;
    if (GetResourceListSpace(list, resource, &index, &emptySlot))
        return INTEGER_VAL((int)index);
    else if (emptySlot) (*list)[index] = resource; else list->push_back(resource);

    ResourceStream* stream = ResourceStream::New(filename);
    if (!stream)
        return INTEGER_VAL(-1);
    resource->AsSprite = FontFace::SpriteFromFont(stream, pixel_sz, filename);
    stream->Close();

    return INTEGER_VAL((int)index);
}
/***
 * Resources.LoadShader
 * \desc
 * \return
 * \ns Resources
 */
VMValue Resources_LoadShader(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);

    char* filename_v = GetString(args, 0, threadID);
    char* filename_f = GetString(args, 1, threadID);

    GLShader* shader;
    size_t found = Scene::ShaderList.size();
    for (size_t i = 0; i < found; i++) {
        shader = Scene::ShaderList[i];
        if (!strcmp(shader->FilenameV, filename_v) &&
            !strcmp(shader->FilenameF, filename_f)) {
            found = i;
            break;
        }
    }
    if (found == Scene::ShaderList.size()) {
        ResourceStream* streamV = ResourceStream::New(filename_v);
        if (!streamV) {
            if (BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "Resource File \"%s\" does not exist.", filename_v) == ERROR_RES_CONTINUE)
                BytecodeObjectManager::Threads[threadID].ReturnFromNative();
        }

        size_t lenV = streamV->Length();
        char*  sourceV = (char*)malloc(lenV);
        streamV->ReadBytes(sourceV, lenV);
        sourceV[lenV] = 0;
        streamV->Close();

        ResourceStream* streamF = ResourceStream::New(filename_f);
        if (!streamF) {
            if (BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "Resource File \"%s\" does not exist.", filename_f) == ERROR_RES_CONTINUE)
                BytecodeObjectManager::Threads[threadID].ReturnFromNative();
        }

        size_t lenF = streamF->Length();
        char*  sourceF = (char*)malloc(lenF);
        streamF->ReadBytes(sourceF, lenF);
        sourceF[lenF] = 0;
        streamF->Close();

        MemoryStream* streamMV = MemoryStream::New(sourceV, lenV);
        MemoryStream* streamMF = MemoryStream::New(sourceF, lenF);

        if (Graphics::Internal.Init == GLRenderer::Init) {
            shader = new GLShader(streamMV, streamMF);
        }
        else {
            free(sourceV);
            free(sourceF);
            streamMV->Close();
            streamMF->Close();
            return INTEGER_VAL((int)-1);
        }

        free(sourceV);
        free(sourceF);
        streamMV->Close();
        streamMF->Close();

        strcpy(shader->FilenameV, filename_v);
        strcpy(shader->FilenameF, filename_f);

        Scene::ShaderList.push_back(shader);
    }

    return INTEGER_VAL((int)found);
}
/***
 * Resources.LoadModel
 * \desc
 * \return
 * \ns Resources
 */
VMValue Resources_LoadModel(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    printf("Filename: %s (%s)\n", GetString(args, 0, threadID), GetDecimal(args, 1, threadID) == 0 ? "SCOPE_SCENE" : "SCOPE_GAME");
    return INTEGER_VAL(-1);
}
/***
 * Resources.LoadMusic
 * \desc
 * \return
 * \ns Resources
 */
VMValue Resources_LoadMusic(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    char*  filename = GetString(args, 0, threadID);

    ResourceType* resource = new ResourceType;
    resource->FilenameHash = CRC32::EncryptString(filename);
    resource->UnloadPolicy = GetInteger(args, 1, threadID);

    size_t index = 0;
    bool emptySlot = false;
    vector<ResourceType*>* list = &Scene::MusicList;
    if (GetResourceListSpace(list, resource, &index, &emptySlot))
        return INTEGER_VAL((int)index);
    else if (emptySlot) (*list)[index] = resource; else list->push_back(resource);

    resource->AsMusic = new ISound(filename);
    return INTEGER_VAL((int)index);
}
/***
 * Resources.LoadSound
 * \desc
 * \return
 * \ns Resources
 */
VMValue Resources_LoadSound(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    char*  filename = GetString(args, 0, threadID);

    ResourceType* resource = new ResourceType;
    resource->FilenameHash = CRC32::EncryptString(filename);
    resource->UnloadPolicy = GetInteger(args, 1, threadID);

    size_t index = 0;
    bool emptySlot = false;
    vector<ResourceType*>* list = &Scene::SoundList;
    if (GetResourceListSpace(list, resource, &index, &emptySlot))
        return INTEGER_VAL((int)index);
    else if (emptySlot) (*list)[index] = resource; else list->push_back(resource);

    resource->AsMusic = new ISound(filename);
    return INTEGER_VAL((int)index);
}
/***
 * Resources.LoadVideo
 * \desc
 * \return
 * \ns Resources
 */
VMValue Resources_LoadVideo(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    char*  filename = GetString(args, 0, threadID);

    ResourceType* resource = new ResourceType;
    resource->FilenameHash = CRC32::EncryptString(filename);
    resource->UnloadPolicy = GetInteger(args, 1, threadID);

    // size_t listSz = Scene::MediaList.size();
    // for (size_t i = 0; i < listSz; i++)
    //     if (Scene::MediaList[i]->FilenameHash == resource->FilenameHash)
    //         return INTEGER_VAL((int)i);
    size_t index = 0;
    bool emptySlot = false;
    vector<ResourceType*>* list = &Scene::MediaList;
    if (GetResourceListSpace(list, resource, &index, &emptySlot))
        return INTEGER_VAL((int)index);
    else if (emptySlot) (*list)[index] = resource; else list->push_back(resource);

    #ifndef NO_LIBAV
    Texture* VideoTexture = NULL;
    MediaSource* Source = NULL;
    MediaPlayer* Player = NULL;

    Stream* stream = NULL;
    if (strncmp(filename, "file:", 5) == 0)
        stream = FileStream::New(filename + 5, FileStream::READ_ACCESS);
    else
        stream = ResourceStream::New(filename);
    if (!stream) {
        Log::Print(Log::LOG_ERROR, "Couldn't open file '%s'!", filename);
        delete resource;
        (*list)[index] = NULL;
        return INTEGER_VAL(-1);
    }

    Source = MediaSource::CreateSourceFromStream(stream);
    if (!Source) {
        delete resource;
        (*list)[index] = NULL;
        return INTEGER_VAL(-1);
    }

    Player = MediaPlayer::Create(Source,
        Source->GetBestStream(MediaSource::STREAMTYPE_VIDEO),
        Source->GetBestStream(MediaSource::STREAMTYPE_AUDIO),
        Source->GetBestStream(MediaSource::STREAMTYPE_SUBTITLE),
        Scene::Views[0].Width, Scene::Views[0].Height);
    if (!Player) {
        Source->Close();
        delete resource;
        (*list)[index] = NULL;
        return INTEGER_VAL(-1);
    }

    PlayerInfo playerInfo;
    Player->GetInfo(&playerInfo);
    VideoTexture = Graphics::CreateTexture(playerInfo.Video.Output.Format, SDL_TEXTUREACCESS_STATIC, playerInfo.Video.Output.Width, playerInfo.Video.Output.Height);
    if (!VideoTexture) {
        Player->Close();
        Source->Close();
        delete resource;
        (*list)[index] = NULL;
        return INTEGER_VAL(-1);
    }

    if (Player->GetVideoStream() > -1) {
        Log::Print(Log::LOG_WARN, "VIDEO STREAM:");
        Log::Print(Log::LOG_INFO, "    Resolution:  %d x %d", playerInfo.Video.Output.Width, playerInfo.Video.Output.Height);
    }
    if (Player->GetAudioStream() > -1) {
        Log::Print(Log::LOG_WARN, "AUDIO STREAM:");
        Log::Print(Log::LOG_INFO, "    Sample Rate: %d", playerInfo.Audio.Output.SampleRate);
        Log::Print(Log::LOG_INFO, "    Bit Depth:   %d-bit", playerInfo.Audio.Output.Format & 0xFF);
        Log::Print(Log::LOG_INFO, "    Channels:    %d", playerInfo.Audio.Output.Channels);
    }

    MediaBag* newMediaBag = new MediaBag;
    newMediaBag->Source = Source;
    newMediaBag->Player = Player;
    newMediaBag->VideoTexture = VideoTexture;

    resource->AsMedia = newMediaBag;
    return INTEGER_VAL((int)index);
    #endif
    return INTEGER_VAL(-1);
}
/***
 * Resources.FileExists
 * \desc
 * \return
 * \ns Resources
 */
VMValue Resources_FileExists(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    char*  filename = GetString(args, 0, threadID);
    Stream* reader = ResourceStream::New(filename);
    if (reader) {
        reader->Close();
        return INTEGER_VAL(true);
    }
    return INTEGER_VAL(false);
}

/***
 * Resources.UnloadImage
 * \desc
 * \return
 * \ns Resources
 */
VMValue Resources_UnloadImage(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int index = GetInteger(args, 0, threadID);

    ResourceType* resource = Scene::ImageList[index];

    if (!resource)
        return INTEGER_VAL(false);
    delete resource;

    if (!resource->AsImage)
        return INTEGER_VAL(false);
    delete resource->AsImage;

    Scene::ImageList[index] = NULL;

    return INTEGER_VAL(true);
}
// #endregion

// #region Scene
#define TILE_FLIPX_MASK 0x80000000U
#define TILE_FLIPY_MASK 0x40000000U
// #define TILE_DIAGO_MASK 0x20000000U
#define TILE_COLLA_MASK 0x30000000U
#define TILE_COLLB_MASK 0x0C000000U
#define TILE_COLLC_MASK 0x03000000U
#define TILE_IDENT_MASK 0x00FFFFFFU
#define CHECK_TILE_LAYER_POS_BOUNDS() if (layer < 0 || layer >= (int)Scene::Layers.size() || x < 0 || y < 0 || x >= Scene::Layers[layer].Width || y >= Scene::Layers[layer].Height) return NULL_VAL;

/***
 * Scene.Load
 * \desc
 * \return
 * \ns Scene
 */
VMValue Scene_Load(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    char* filename = GetString(args, 0, threadID);

    strcpy(Scene::NextScene, filename);
    Scene::NextScene[strlen(filename)] = 0;

    return NULL_VAL;
}
/***
 * Scene.LoadTileCollisions
 * \desc
 * \return
 * \ns Scene
 */
VMValue Scene_LoadTileCollisions(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    char* filename = GetString(args, 0, threadID);
    Scene::LoadTileCollisions(filename);
    return NULL_VAL;
}
/***
 * Scene.Restart
 * \desc
 * \return
 * \ns Scene
 */
VMValue Scene_Restart(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    Scene::DoRestart = true;
    return NULL_VAL;
}
/***
 * Scene.GetLayerCount
 * \desc
 * \return
 * \ns Scene
 */
VMValue Scene_GetLayerCount(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    return INTEGER_VAL((int)Scene::Layers.size());
}
/***
 * Scene.GetLayerIndex
 * \desc
 * \return
 * \ns Scene
 */
VMValue Scene_GetLayerIndex(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    char* layername = GetString(args, 0, threadID);
    for (size_t i = 0; i < Scene::Layers.size(); i++) {
        if (strcmp(Scene::Layers[i].Name, layername) == 0)
            return INTEGER_VAL((int)i);
    }
    return INTEGER_VAL(-1);
}
/***
 * Scene.GetName
 * \desc
 * \return
 * \ns Scene
 */
VMValue Scene_GetName(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    return OBJECT_VAL(CopyString(Scene::CurrentScene, strlen(Scene::CurrentScene)));
}
/***
 * Scene.GetWidth
 * \desc
 * \return
 * \ns Scene
 */
VMValue Scene_GetWidth(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    int v = 0;
    if (Scene::Layers.size() > 0)
        v = Scene::Layers[0].Width;

    for (size_t i = 0; i < Scene::Layers.size(); i++) {
        if (strcmp(Scene::Layers[i].Name, "FG Low") == 0)
            return INTEGER_VAL(Scene::Layers[i].Width);
    }

    return INTEGER_VAL(v);
}
/***
 * Scene.GetHeight
 * \desc
 * \return
 * \ns Scene
 */
VMValue Scene_GetHeight(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    int v = 0;
    if (Scene::Layers.size() > 0)
        v = Scene::Layers[0].Height;

    for (size_t i = 0; i < Scene::Layers.size(); i++) {
        if (strcmp(Scene::Layers[i].Name, "FG Low") == 0)
            return INTEGER_VAL(Scene::Layers[i].Height);
    }

    return INTEGER_VAL(v);
}
/***
 * Scene.GetTileSize
 * \desc
 * \return
 * \ns Scene
 */
VMValue Scene_GetTileSize(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    return INTEGER_VAL(Scene::TileSize);
}
/***
 * Scene.GetTileID
 * \desc
 * \return
 * \ns Scene
 */
VMValue Scene_GetTileID(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);
    int layer  = GetInteger(args, 0, threadID);
    int x = (int)GetDecimal(args, 1, threadID);
    int y = (int)GetDecimal(args, 2, threadID);

    CHECK_TILE_LAYER_POS_BOUNDS();

    return INTEGER_VAL((int)(Scene::Layers[layer].Tiles[x + (y << Scene::Layers[layer].WidthInBits)] & TILE_IDENT_MASK));
}
/***
 * Scene.GetTileFlipX
 * \desc
 * \return
 * \ns Scene
 */
VMValue Scene_GetTileFlipX(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);
    int layer  = GetInteger(args, 0, threadID);
    int x = (int)GetDecimal(args, 1, threadID);
    int y = (int)GetDecimal(args, 2, threadID);

    CHECK_TILE_LAYER_POS_BOUNDS();

    return INTEGER_VAL(!!(Scene::Layers[layer].Tiles[x + (y << Scene::Layers[layer].WidthInBits)] & TILE_FLIPX_MASK));
}
/***
 * Scene.GetTileFlipY
 * \desc
 * \return
 * \ns Scene
 */
VMValue Scene_GetTileFlipY(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);
    int layer  = GetInteger(args, 0, threadID);
    int x = (int)GetDecimal(args, 1, threadID);
    int y = (int)GetDecimal(args, 2, threadID);

    CHECK_TILE_LAYER_POS_BOUNDS();

    return INTEGER_VAL(!!(Scene::Layers[layer].Tiles[x + (y << Scene::Layers[layer].WidthInBits)] & TILE_FLIPY_MASK));
}
/***
 * Scene.SetTile
 * \desc
 * \return
 * \ns Scene
 */
VMValue Scene_SetTile(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_AT_LEAST_ARGCOUNT(6);
    int layer  = GetInteger(args, 0, threadID);
    int x = (int)GetDecimal(args, 1, threadID);
    int y = (int)GetDecimal(args, 2, threadID);
    int tileID = GetInteger(args, 3, threadID);
    int flip_x = GetInteger(args, 4, threadID);
    int flip_y = GetInteger(args, 5, threadID);
    // Optionals
    int collA = TILE_COLLA_MASK, collB = TILE_COLLB_MASK;
    if (argCount == 7) {
        collA = collB = GetInteger(args, 6, threadID);
        // collA <<= 28;
        // collB <<= 26;
    }
    else if (argCount == 8) {
        collA = GetInteger(args, 6, threadID);
        collB = GetInteger(args, 7, threadID);
    }

    CHECK_TILE_LAYER_POS_BOUNDS();

    Uint32* tile = &Scene::Layers[layer].Tiles[x + (y << Scene::Layers[layer].WidthInBits)];

    *tile = tileID & TILE_IDENT_MASK;
    if (flip_x)
        *tile |= TILE_FLIPX_MASK;
    if (flip_y)
        *tile |= TILE_FLIPY_MASK;

    *tile |= collA;
    *tile |= collB;

    Scene::UpdateTileBatch(layer, x / 8, y / 8);

    Scene::AnyLayerTileChange = true;

    return NULL_VAL;
}
/***
 * Scene.SetTileCollisionSides
 * \desc
 * \return
 * \ns Scene
 */
VMValue Scene_SetTileCollisionSides(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(5);
    int layer  = GetInteger(args, 0, threadID);
    int x = (int)GetDecimal(args, 1, threadID);
    int y = (int)GetDecimal(args, 2, threadID);
    int collA = GetInteger(args, 3, threadID) << 28;
    int collB = GetInteger(args, 4, threadID) << 26;

    CHECK_TILE_LAYER_POS_BOUNDS();

    Uint32* tile = &Scene::Layers[layer].Tiles[x + (y << Scene::Layers[layer].WidthInBits)];

    *tile &= TILE_FLIPX_MASK | TILE_FLIPY_MASK | TILE_IDENT_MASK;
    *tile |= collA;
    *tile |= collB;

    Scene::AnyLayerTileChange = true;

    return NULL_VAL;
}
/***
 * Scene.SetPaused
 * \desc
 * \return
 * \ns Scene
 */
VMValue Scene_SetPaused(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    Scene::Paused = GetInteger(args, 0, threadID);
    return NULL_VAL;
}
/***
 * Scene.SetLayerVisible
 * \desc
 * \return
 * \ns Scene
 */
VMValue Scene_SetLayerVisible(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    int index = GetInteger(args, 0, threadID);
    int visible = GetInteger(args, 1, threadID);
    // for (size_t i = 0; i < Scene::Layers.size(); i++) {
    //     if (strcmp(Scene::Layers[i].Name, layername) == 0)
    //         return INTEGER_VAL((int)i);
    // }
    Scene::Layers[index].Visible = visible;
    return NULL_VAL;
}
/***
 * Scene.SetLayerOffsetPosition
 * \desc
 * \return
 * \ns Scene
 */
VMValue Scene_SetLayerOffsetPosition(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);
    int index = GetInteger(args, 0, threadID);
    int offsetX = (int)GetDecimal(args, 1, threadID);
    int offsetY = (int)GetDecimal(args, 2, threadID);
    Scene::Layers[index].OffsetX = offsetX;
    Scene::Layers[index].OffsetY = offsetY;
    return NULL_VAL;
}
/***
 * Scene.SetLayerDrawGroup
 * \desc
 * \return
 * \ns Scene
 */
VMValue Scene_SetLayerDrawGroup(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    int index = GetInteger(args, 0, threadID);
    int drawg = GetInteger(args, 1, threadID);
    Scene::Layers[index].DrawGroup = drawg % Scene::PriorityPerLayer;
    return NULL_VAL;
}
/***
 * Scene.IsPaused
 * \desc
 * \return
 * \ns Scene
 */
VMValue Scene_IsPaused(int argCount, VMValue* args, Uint32 threadID) {
    return INTEGER_VAL((int)Scene::Paused);
}
// #endregion

// #region Shader
/***
 * Shader.Set
 * \desc
 * \return
 * \ns Shader
 */
VMValue Shader_Set(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    if (IS_ARRAY(args[0])) {
        ObjArray* array = GetArray(args, 0, threadID);
        Graphics::UseShader(array);
        return NULL_VAL;
    }

    int   arg1 = GetInteger(args, 0, threadID);

	if (!(arg1 >= 0 && arg1 < (int)Scene::ShaderList.size())) return NULL_VAL;

    Graphics::UseShader(Scene::ShaderList[arg1]);
    return NULL_VAL;
}
/***
 * Shader.GetUniform
 * \desc
 * \return
 * \ns Shader
 */
VMValue Shader_GetUniform(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    int   arg1 = GetInteger(args, 0, threadID);
    char* arg2 = GetString(args, 1, threadID);

    if (!(arg1 >= 0 && arg1 < (int)Scene::ShaderList.size())) return INTEGER_VAL(-1);

    return INTEGER_VAL(Scene::ShaderList[arg1]->GetUniformLocation(arg2));
}
/***
 * Shader.SetUniformI
 * \desc
 * \return
 * \ns Shader
 */
VMValue Shader_SetUniformI(int argCount, VMValue* args, Uint32 threadID) {
    // CHECK_AT_LEAST_ARGCOUNT(1);
    // int   arg1 = GetInteger(args, 0, threadID);
    // for (int i = 1; i < argCount; i++) {
    //
    // }

    return NULL_VAL;
}
/***
 * Shader.SetUniformF
 * \desc
 * \return
 * \ns Shader
 */
VMValue Shader_SetUniformF(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_AT_LEAST_ARGCOUNT(1);
    int   arg1 = GetInteger(args, 0, threadID);

    float* values = (float*)malloc((argCount - 1) * sizeof(float));
    for (int i = 1; i < argCount; i++) {
        values[i - 1] = GetDecimal(args, i, threadID);
    }
    Graphics::Internal.SetUniformF(arg1, argCount - 1, values);
    free(values);
    return NULL_VAL;
}
/***
 * Shader.SetUniform3x3
 * \desc
 * \return
 * \ns Shader
 */
VMValue Shader_SetUniform3x3(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(9);
    for (int i = 0; i < 9; i++) {
        // GetDecimal(args, i, threadID);
    }
    return NULL_VAL;
}
/***
 * Shader.SetUniform4x4
 * \desc
 * \return
 * \ns Shader
 */
VMValue Shader_SetUniform4x4(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(16);
    for (int i = 0; i < 16; i++) {
        // GetDecimal(args, i, threadID);
    }
    return NULL_VAL;
}
/***
 * Shader.SetUniformTexture
 * \desc
 * \return
 * \ns Shader
 */
VMValue Shader_SetUniformTexture(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);
    int   uniform_index = GetInteger(args, 0, threadID);
    int   texture_index = GetInteger(args, 1, threadID);
    int   slot = GetInteger(args, 2, threadID);
    Texture* texture = Graphics::TextureMap->Get(texture_index);

    Graphics::Internal.SetUniformTexture(texture, uniform_index, slot);
    return NULL_VAL;
}
/***
 * Shader.Unset
 * \desc
 * \return
 * \ns Shader
 */
VMValue Shader_Unset(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    Graphics::UseShader(NULL);
    return NULL_VAL;
}
// #endregion

// #region SocketClient
WebSocketClient* client = NULL;
/***
 * SocketClient.Open
 * \desc
 * \return
 * \ns SocketClient
 */
VMValue SocketClient_Open(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    char* url = GetString(args, 0, threadID);

    client = WebSocketClient::New(url);
    if (!client)
        return INTEGER_VAL(false);

    return INTEGER_VAL(true);
}
/***
 * SocketClient.Close
 * \desc
 * \return
 * \ns SocketClient
 */
VMValue SocketClient_Close(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    if (!client)
        return NULL_VAL;

    client->Close();
    client = NULL;

    return NULL_VAL;
}
/***
 * SocketClient.IsOpen
 * \desc
 * \return
 * \ns SocketClient
 */
VMValue SocketClient_IsOpen(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    if (!client || client->readyState != WebSocketClient::OPEN)
        return INTEGER_VAL(false);

    return INTEGER_VAL(true);
}
/***
 * SocketClient.Poll
 * \desc
 * \return
 * \ns SocketClient
 */
VMValue SocketClient_Poll(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int timeout = GetInteger(args, 0, threadID);

    if (!client)
        return INTEGER_VAL(false);

    client->Poll(timeout);
    return INTEGER_VAL(true);
}
/***
 * SocketClient.BytesToRead
 * \desc
 * \return
 * \ns SocketClient
 */
VMValue SocketClient_BytesToRead(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    if (!client)
        return INTEGER_VAL(0);

    return INTEGER_VAL((int)client->BytesToRead());
}
/***
 * SocketClient.ReadDecimal
 * \desc
 * \return
 * \ns SocketClient
 */
VMValue SocketClient_ReadDecimal(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    if (!client)
        return NULL_VAL;

    return DECIMAL_VAL(client->ReadFloat());
}
/***
 * SocketClient.ReadInteger
 * \desc
 * \return
 * \ns SocketClient
 */
VMValue SocketClient_ReadInteger(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    if (!client)
        return NULL_VAL;

    return INTEGER_VAL(client->ReadSint32());
}
/***
 * SocketClient.ReadString
 * \desc
 * \return
 * \ns SocketClient
 */
VMValue SocketClient_ReadString(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    if (!client)
        return NULL_VAL;

    if (BytecodeObjectManager::Lock()) {
        char* str = client->ReadString();
        ObjString* objStr = TakeString(str, strlen(str));

        BytecodeObjectManager::Unlock();
        return OBJECT_VAL(objStr);
    }
    return NULL_VAL;
}
/***
 * SocketClient.WriteDecimal
 * \desc
 * \return
 * \ns SocketClient
 */
VMValue SocketClient_WriteDecimal(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    float value = GetDecimal(args, 0, threadID);
    if (!client)
        return NULL_VAL;

    client->SendBinary(&value, sizeof(value));
    return NULL_VAL;
}
/***
 * SocketClient.WriteInteger
 * \desc
 * \return
 * \ns SocketClient
 */
VMValue SocketClient_WriteInteger(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int value = GetInteger(args, 0, threadID);
    if (!client)
        return NULL_VAL;

    client->SendBinary(&value, sizeof(value));
    return NULL_VAL;
}
/***
 * SocketClient.WriteString
 * \desc
 * \return
 * \ns SocketClient
 */
VMValue SocketClient_WriteString(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    char* value = GetString(args, 0, threadID);
    if (!client)
        return NULL_VAL;

    client->SendText(value);
    return NULL_VAL;
}
// #endregion

// #region Sound
/***
 * Sound.Play
 * \desc
 * \return
 * \ns Sound
 */
VMValue Sound_Play(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    ISound* audio = Scene::SoundList[GetInteger(args, 0, threadID)]->AsSound;
    int channel = GetInteger(args, 0, threadID);
    AudioManager::SetSound(channel % AudioManager::SoundArrayLength, audio);
    return NULL_VAL;
}
/***
 * Sound.Loop
 * \desc
 * \return
 * \ns Sound
 */
VMValue Sound_Loop(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    ISound* audio = Scene::SoundList[GetInteger(args, 0, threadID)]->AsSound;
    int channel = GetInteger(args, 0, threadID);
    AudioManager::SetSound(channel % AudioManager::SoundArrayLength, audio, true, 0);
    return NULL_VAL;
}
/***
 * Sound.Stop
 * \desc
 * \return
 * \ns Sound
 */
VMValue Sound_Stop(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);

    int channel = GetInteger(args, 0, threadID);

    AudioManager::AudioStop(channel % AudioManager::SoundArrayLength);
    return NULL_VAL;
}
/***
 * Sound.Pause
 * \desc
 * \return
 * \ns Sound
 */
VMValue Sound_Pause(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int channel = GetInteger(args, 0, threadID);
    AudioManager::AudioPause(channel % AudioManager::SoundArrayLength);
    return NULL_VAL;
}
/***
 * Sound.Resume
 * \desc
 * \return
 * \ns Sound
 */
VMValue Sound_Resume(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int channel = GetInteger(args, 0, threadID);
    AudioManager::AudioUnpause(channel % AudioManager::SoundArrayLength);
    return NULL_VAL;
}
/***
 * Sound.StopAll
 * \desc
 * \return
 * \ns Sound
 */
VMValue Sound_StopAll(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    AudioManager::AudioStopAll();
    return NULL_VAL;
}
/***
 * Sound.PauseAll
 * \desc
 * \return
 * \ns Sound
 */
VMValue Sound_PauseAll(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    AudioManager::AudioPauseAll();
    return NULL_VAL;
}
/***
 * Sound.ResumeAll
 * \desc
 * \return
 * \ns Sound
 */
VMValue Sound_ResumeAll(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    AudioManager::AudioUnpauseAll();
    return NULL_VAL;
}
// #endregion

// #region Sprite
/***
 * Sprite.GetAnimationCount
 * \desc
 * \return
 * \ns Sprite
 */
VMValue Sprite_GetAnimationCount(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    ISprite* sprite = Scene::SpriteList[GetInteger(args, 0, threadID)]->AsSprite;
    return INTEGER_VAL((int)sprite->Animations.size());
}
/***
 * Sprite.GetFrameLoopIndex
 * \desc
 * \return
 * \ns Sprite
 */
VMValue Sprite_GetFrameLoopIndex(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    ISprite* sprite = Scene::SpriteList[GetInteger(args, 0, threadID)]->AsSprite;
    int animation = GetInteger(args, 1, threadID);
    return INTEGER_VAL(sprite->Animations[animation].FrameToLoop);
}
/***
 * Sprite.GetFrameCount
 * \desc
 * \return
 * \ns Sprite
 */
VMValue Sprite_GetFrameCount(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    ISprite* sprite = Scene::SpriteList[GetInteger(args, 0, threadID)]->AsSprite;
    int animation = GetInteger(args, 1, threadID);
    return INTEGER_VAL((int)sprite->Animations[animation].Frames.size());
}
/***
 * Sprite.GetFrameDuration
 * \desc
 * \return
 * \ns Sprite
 */
VMValue Sprite_GetFrameDuration(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);
    ISprite* sprite = Scene::SpriteList[GetInteger(args, 0, threadID)]->AsSprite;
    int animation = GetInteger(args, 1, threadID);
    int frame = GetInteger(args, 2, threadID);
    return INTEGER_VAL(sprite->Animations[animation].Frames[frame].Duration);
}
/***
 * Sprite.GetFrameSpeed
 * \desc
 * \return
 * \ns Sprite
 */
VMValue Sprite_GetFrameSpeed(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    ISprite* sprite = Scene::SpriteList[GetInteger(args, 0, threadID)]->AsSprite;
    int animation = GetInteger(args, 1, threadID);
    return INTEGER_VAL(sprite->Animations[animation].AnimationSpeed);
}
// #endregion

// #region String
/***
 * String.Split
 * \desc
 * \return
 * \ns String
 */
VMValue String_Split(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    // char* string = GetString(args, 0, threadID);
    // char* delimt = GetString(args, 1, threadID);

    if (BytecodeObjectManager::Lock()) {
        ObjArray* array = NewArray();
        int       length = 1;
        for (int i = 0; i < length; i++) {
            ObjString* part = AllocString(16);
            array->Values->push_back(OBJECT_VAL(part));
        }

        BytecodeObjectManager::Unlock();
        return OBJECT_VAL(array);
    }
    return NULL_VAL;
}
/***
 * String.CharAt
 * \desc
 * \return
 * \ns String
 */
VMValue String_CharAt(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    char* string = GetString(args, 0, threadID);
    int   n = GetInteger(args, 1, threadID);
    return INTEGER_VAL((Uint8)string[n]);
}
/***
 * String.Length
 * \desc
 * \return
 * \ns String
 */
VMValue String_Length(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    char* string = GetString(args, 0, threadID);
    return INTEGER_VAL((int)strlen(string));
}
/***
 * String.Compare
 * \desc
 * \return
 * \ns String
 */
VMValue String_Compare(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    char* stringA = GetString(args, 0, threadID);
    char* stringB = GetString(args, 1, threadID);
    return INTEGER_VAL((int)strcmp(stringA, stringB));
}
/***
 * String.IndexOf
 * \desc
 * \return
 * \ns String
 */
VMValue String_IndexOf(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    char* string = GetString(args, 0, threadID);
    char* substring = GetString(args, 1, threadID);
    char* find = strstr(string, substring);
    if (!find)
        return INTEGER_VAL(-1);
    return INTEGER_VAL((int)(find - string));
}
/***
 * String.Contains
 * \desc
 * \return
 * \ns String
 */
VMValue String_Contains(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    char* string = GetString(args, 0, threadID);
    char* substring = GetString(args, 1, threadID);
    return INTEGER_VAL((int)(!!strstr(string, substring)));
}
/***
 * String.Substring
 * \desc
 * \return
 * \ns String
 */
VMValue String_Substring(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);
    char* string = GetString(args, 0, threadID);
    int index = GetInteger(args, 1, threadID);
    int length = GetInteger(args, 2, threadID);

    int strln = strlen(string);
    if (length > strln - index || length == -1)
        length = strln - index;

    VMValue obj = NULL_VAL;
    if (BytecodeObjectManager::Lock()) {
        obj = OBJECT_VAL(CopyString(string + index, length));
        BytecodeObjectManager::Unlock();
    }
    return obj;
}
/***
 * String.ToUpperCase
 * \desc
 * \return
 * \ns String
 */
VMValue String_ToUpperCase(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    char* string = GetString(args, 0, threadID);

    VMValue obj = NULL_VAL;
    if (BytecodeObjectManager::Lock()) {
        ObjString* objStr = CopyString(string, strlen(string));
        for (char* a = objStr->Chars; *a; a++) {
            if (*a >= 'a' && *a <= 'z')
                *a += 'A' - 'a';
            else if (*a >= 'a' && *a <= 'z')
                *a += 'A' - 'a';
        }
        obj = OBJECT_VAL(objStr);
        BytecodeObjectManager::Unlock();
    }
    return obj;
}
/***
 * String.ToLowerCase
 * \desc
 * \return
 * \ns String
 */
VMValue String_ToLowerCase(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    char* string = GetString(args, 0, threadID);

    VMValue obj = NULL_VAL;
    if (BytecodeObjectManager::Lock()) {
        ObjString* objStr = CopyString(string, strlen(string));
        for (char* a = objStr->Chars; *a; a++) {
            if (*a >= 'A' && *a <= 'Z')
                *a += 'a' - 'A';
        }
        obj = OBJECT_VAL(objStr);
        BytecodeObjectManager::Unlock();
    }
    return obj;
}
/***
 * String.LastIndexOf
 * \desc
 * \return
 * \ns String
 */
VMValue String_LastIndexOf(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    char* string = GetString(args, 0, threadID);
    char* substring = GetString(args, 1, threadID);
    size_t string_len = strlen(string);
    size_t substring_len = strlen(substring);
    if (string_len < substring_len)
        return INTEGER_VAL(-1);

    char* find = NULL;
    for (char* start = string + string_len - substring_len; start >= string; start--) {
        if (memcmp(start, substring, substring_len) == 0) {
            find = start;
            break;
        }
    }
    if (!find)
        return INTEGER_VAL(-1);
    return INTEGER_VAL((int)(find - string));
}
/***
 * String.ParseInteger
 * \desc
 * \return
 * \ns String
 */
VMValue String_ParseInteger(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    char* string = GetString(args, 0, threadID);
    return INTEGER_VAL((int)strtol(string, NULL, 10));
}
/***
 * String.ParseDecimal
 * \desc
 * \return
 * \ns String
 */
VMValue String_ParseDecimal(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    char* string = GetString(args, 0, threadID);
    return DECIMAL_VAL((float)strtod(string, NULL));
}
// #endregion

// #region Texture
/***
 * Texture.Create
 * \desc
 * \return
 * \ns Texture
 */
VMValue Texture_Create(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    int width = GetInteger(args, 0, threadID);
    int height = GetInteger(args, 1, threadID);
    Texture* texture = Graphics::CreateTexture(SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, width, height);
    return INTEGER_VAL((int)texture->ID);
}
/***
 * Texture.FromSprite
 * \desc
 * \return
 * \ns Texture
 */
VMValue Texture_FromSprite(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    ISprite* arg1 = Scene::SpriteList[GetInteger(args, 0, threadID)]->AsSprite;
    return INTEGER_VAL((int)arg1->Spritesheets[0]->ID);
}
/***
 * Texture.FromImage
 * \desc
 * \return
 * \ns Texture
 */
VMValue Texture_FromImage(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    Image* arg1 = Scene::ImageList[GetInteger(args, 0, threadID)]->AsImage;
    return INTEGER_VAL((int)arg1->TexturePtr->ID);
}
/***
 * Texture.SetInterpolation
 * \desc
 * \return
 * \ns Texture
 */
VMValue Texture_SetInterpolation(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int interpolate = GetInteger(args, 0, threadID);
    Graphics::SetTextureInterpolation(interpolate);
    return NULL_VAL;
}
// #endregion

// #region Touch
/***
 * Touch.GetX
 * \desc
 * \return
 * \ns Touch
 */
VMValue Touch_GetX(int argCount, VMValue* args, Uint32 threadID) {
    int touch_index = GetInteger(args, 0, threadID);
    return DECIMAL_VAL(InputManager::TouchGetX(touch_index));
}
/***
 * Touch.GetY
 * \desc
 * \return
 * \ns Touch
 */
VMValue Touch_GetY(int argCount, VMValue* args, Uint32 threadID) {
    int touch_index = GetInteger(args, 0, threadID);
    return DECIMAL_VAL(InputManager::TouchGetY(touch_index));
}
/***
 * Touch.IsDown
 * \desc
 * \return
 * \ns Touch
 */
VMValue Touch_IsDown(int argCount, VMValue* args, Uint32 threadID) {
    int touch_index = GetInteger(args, 0, threadID);
    return INTEGER_VAL(InputManager::TouchIsDown(touch_index));
}
/***
 * Touch.IsPressed
 * \desc
 * \return
 * \ns Touch
 */
VMValue Touch_IsPressed(int argCount, VMValue* args, Uint32 threadID) {
    int touch_index = GetInteger(args, 0, threadID);
    return INTEGER_VAL(InputManager::TouchIsPressed(touch_index));
}
/***
 * Touch.IsReleased
 * \desc
 * \return
 * \ns Touch
 */
VMValue Touch_IsReleased(int argCount, VMValue* args, Uint32 threadID) {
    int touch_index = GetInteger(args, 0, threadID);
    return INTEGER_VAL(InputManager::TouchIsReleased(touch_index));
}
// #endregion

// #region TileCollision
/***
 * TileCollision.Point
 * \desc
 * \return
 * \ns TileCollision
 */
VMValue TileCollision_Point(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    int x = (int)std::floor(GetDecimal(args, 0, threadID));
    int y = (int)std::floor(GetDecimal(args, 1, threadID));

    // 15, or 0b1111
    return INTEGER_VAL(Scene::CollisionAt(x, y, 0, 15, NULL) >= 0);
}
/***
 * TileCollision.PointExtended
 * \desc
 * \return
 * \ns TileCollision
 */
VMValue TileCollision_PointExtended(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(4);
    int x = (int)std::floor(GetDecimal(args, 0, threadID));
    int y = (int)std::floor(GetDecimal(args, 1, threadID));
    int collisionField = GetInteger(args, 2, threadID);
    int collisionSide = GetInteger(args, 3, threadID);

    return INTEGER_VAL(Scene::CollisionAt(x, y, collisionField, collisionSide, NULL));
}
// #endregion

// #region TileInfo
/***
 * TileInfo.IsEmptySpace
 * \desc
 * \return
 * \ns TileInfo
 */
VMValue TileInfo_IsEmptySpace(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    int tileID = GetInteger(args, 0, threadID);
    int colplane = GetInteger(args, 1, threadID);

    if (colplane == 0)
        return INTEGER_VAL(Scene::TileCfgA[tileID].Config[1] || !Scene::TileCfgA[tileID].HasCollision);

    return INTEGER_VAL(Scene::TileCfgB[tileID].Config[1] || !Scene::TileCfgB[tileID].HasCollision);
}
// #endregion

// #region Thread
struct _Thread_Bundle {
    ObjFunction    Callback;
    int            ArgCount;
    int            ThreadIndex;
};

int     _Thread_RunEvent(void* op) {
    _Thread_Bundle* bundle;
    bundle = (_Thread_Bundle*)op;

    VMValue*  args = (VMValue*)(bundle + 1);
    VMThread* thread = BytecodeObjectManager::Threads + bundle->ThreadIndex;
    VMValue   callbackVal = OBJECT_VAL(&bundle->Callback);

    // if (bundle->Callback.Method == NULL) {
    //     Log::Print(Log::LOG_ERROR, "No callback.");
    //     BytecodeObjectManager::ThreadCount--;
    //     free(bundle);
    //     return 0;
    // }

    thread->Push(callbackVal);
    for (int i = 0; i < bundle->ArgCount; i++) {
        thread->Push(args[i]);
    }
    thread->RunValue(callbackVal, bundle->ArgCount);

    free(bundle);

    BytecodeObjectManager::ThreadCount--;
    Log::Print(Log::LOG_IMPORTANT, "Thread %d closed.", BytecodeObjectManager::ThreadCount);
    return 0;
}
/***
 * Thread.RunEvent
 * \desc
 * \return
 * \ns Thread
 */
VMValue Thread_RunEvent(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_AT_LEAST_ARGCOUNT(1);

    ObjFunction* callback = NULL;
    if (IS_BOUND_METHOD(args[0])) {
        callback = GetBoundMethod(args, 0, threadID)->Method;
    }
    else if (IS_FUNCTION(args[0])) {
        callback = AS_FUNCTION(args[0]);
    }

    if (callback == NULL) {
        Compiler::PrintValue(args[0]);
        printf("\n");
        Log::Print(Log::LOG_ERROR, "No callback.");
        return NULL_VAL;
    }

    int subArgCount = argCount - 1;

    _Thread_Bundle* bundle = (_Thread_Bundle*)malloc(sizeof(_Thread_Bundle) + subArgCount * sizeof(VMValue));
    bundle->Callback = *callback;
    bundle->Callback.Object.Next = NULL;
    bundle->ArgCount = subArgCount;
    bundle->ThreadIndex = BytecodeObjectManager::ThreadCount++;
    if (subArgCount > 0)
        memcpy(bundle + 1, args + 1, subArgCount * sizeof(VMValue));

    // SDL_DetachThread
    SDL_CreateThread(_Thread_RunEvent, "_Thread_RunEvent", bundle);

    return NULL_VAL;
}
/***
 * Thread.Sleep
 * \desc
 * \return
 * \ns Thread
 */
VMValue Thread_Sleep(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    SDL_Delay(GetInteger(args, 0, threadID));
    return NULL_VAL;
}
// #endregion

// #region Video
/***
 * Video.Play
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_Play(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    MediaBag* video = GetVideo(args, 0, threadID);
    #ifndef NO_LIBAV
    video->Player->Play();
    #endif
    return NULL_VAL;
}
/***
 * Video.Pause
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_Pause(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    MediaBag* video = GetVideo(args, 0, threadID);
#ifndef NO_LIBAV
    video->Player->Pause();
#endif
    return NULL_VAL;
}
/***
 * Video.Resume
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_Resume(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    MediaBag* video = GetVideo(args, 0, threadID);
#ifndef NO_LIBAV
    video->Player->Play();
#endif
    return NULL_VAL;
}
/***
 * Video.Stop
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_Stop(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    MediaBag* video = GetVideo(args, 0, threadID);
#ifndef NO_LIBAV
    video->Player->Stop();
    video->Player->Seek(0);
#endif
    return NULL_VAL;
}
/***
 * Video.Close
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_Close(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    MediaBag* video = GetVideo(args, 0, threadID);

    int index = GetInteger(args, 0, threadID);
    ResourceType* resource = Scene::MediaList[index];

    Scene::MediaList[index] = NULL;

#ifndef NO_LIBAV
    video->Source->Close();
    video->Player->Close();
#endif

    if (!resource)
        return NULL_VAL;
    delete resource;

    if (!resource->AsMedia)
        return NULL_VAL;
    delete resource->AsMedia;

    return NULL_VAL;
}
/***
 * Video.IsBuffering
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_IsBuffering(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    MediaBag* video = GetVideo(args, 0, threadID);
#ifndef NO_LIBAV
    return INTEGER_VAL(video->Player->IsOutputEmpty());
#endif
    return NULL_VAL;
}
/***
 * Video.IsPlaying
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_IsPlaying(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    MediaBag* video = GetVideo(args, 0, threadID);
#ifndef NO_LIBAV
    return INTEGER_VAL(video->Player->GetPlayerState() == MediaPlayer::KIT_PLAYING);
#endif
    return NULL_VAL;
}
/***
 * Video.IsPaused
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_IsPaused(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    MediaBag* video = GetVideo(args, 0, threadID);
#ifndef NO_LIBAV
    return INTEGER_VAL(video->Player->GetPlayerState() == MediaPlayer::KIT_PAUSED);
#endif
    return NULL_VAL;
}
/***
 * Video.SetPosition
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_SetPosition(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    MediaBag* video = GetVideo(args, 0, threadID);
    float position = GetDecimal(args, 1, threadID);
#ifndef NO_LIBAV
    video->Player->Seek(position);
#endif
    return NULL_VAL;
}
/***
 * Video.SetBufferDuration
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_SetBufferDuration(int argCount, VMValue* args, Uint32 threadID) {
    return NULL_VAL;
}
/***
 * Video.SetTrackEnabled
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_SetTrackEnabled(int argCount, VMValue* args, Uint32 threadID) {
    return NULL_VAL;
}
/***
 * Video.GetPosition
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_GetPosition(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    MediaBag* video = GetVideo(args, 0, threadID);
#ifndef NO_LIBAV
    return DECIMAL_VAL((float)video->Player->GetPosition());
#endif
    return NULL_VAL;
}
/***
 * Video.GetDuration
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_GetDuration(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    MediaBag* video = GetVideo(args, 0, threadID);
#ifndef NO_LIBAV
    return DECIMAL_VAL((float)video->Player->GetDuration());
#endif
    return NULL_VAL;
}
/***
 * Video.GetBufferDuration
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_GetBufferDuration(int argCount, VMValue* args, Uint32 threadID) {
    return NULL_VAL;
}
/***
 * Video.GetBufferEnd
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_GetBufferEnd(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    MediaBag* video = GetVideo(args, 0, threadID);
#ifndef NO_LIBAV
    return DECIMAL_VAL((float)video->Player->GetBufferPosition());
#endif
    return NULL_VAL;
}
/***
 * Video.GetTrackCount
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_GetTrackCount(int argCount, VMValue* args, Uint32 threadID) {
    return NULL_VAL;
}
/***
 * Video.GetTrackEnabled
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_GetTrackEnabled(int argCount, VMValue* args, Uint32 threadID) {
    return NULL_VAL;
}
/***
 * Video.GetTrackName
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_GetTrackName(int argCount, VMValue* args, Uint32 threadID) {
    return NULL_VAL;
}
/***
 * Video.GetTrackLanguage
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_GetTrackLanguage(int argCount, VMValue* args, Uint32 threadID) {
    return NULL_VAL;
}
/***
 * Video.GetDefaultVideoTrack
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_GetDefaultVideoTrack(int argCount, VMValue* args, Uint32 threadID) {
    return NULL_VAL;
}
/***
 * Video.GetDefaultAudioTrack
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_GetDefaultAudioTrack(int argCount, VMValue* args, Uint32 threadID) {
    return NULL_VAL;
}
/***
 * Video.GetDefaultSubtitleTrack
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_GetDefaultSubtitleTrack(int argCount, VMValue* args, Uint32 threadID) {
    return NULL_VAL;
}
/***
 * Video.GetWidth
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_GetWidth(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    MediaBag* video = GetVideo(args, 0, threadID);
    if (video->VideoTexture)
        return INTEGER_VAL((int)video->VideoTexture->Width);

    return INTEGER_VAL(-1);
}
/***
 * Video.GetHeight
 * \desc
 * \return
 * \ns Video
 */
VMValue Video_GetHeight(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    MediaBag* video = GetVideo(args, 0, threadID);
    if (video->VideoTexture)
        return INTEGER_VAL((int)video->VideoTexture->Height);

    return INTEGER_VAL(-1);
}
// #endregion

// #region View
/***
 * View.SetX
 * \desc
 * \return
 * \ns View
 */
VMValue View_SetX(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    int   view_index = GetInteger(args, 0, threadID);
    float value = GetDecimal(args, 1, threadID);
    if (view_index < 0 || view_index > 7) {
        BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "View Index out of range 0 through 8");
        return NULL_VAL;
    }
    Scene::Views[view_index].X = value;
    return NULL_VAL;
}
/***
 * View.SetY
 * \desc
 * \return
 * \ns View
 */
VMValue View_SetY(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    int   view_index = GetInteger(args, 0, threadID);
    float value = GetDecimal(args, 1, threadID);
    if (view_index < 0 || view_index > 7) {
        BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "View Index out of range 0 through 8");
        return NULL_VAL;
    }
    Scene::Views[view_index].Y = value;
    return NULL_VAL;
}
/***
 * View.SetZ
 * \desc
 * \return
 * \ns View
 */
VMValue View_SetZ(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    int   view_index = GetInteger(args, 0, threadID);
    float value = GetDecimal(args, 1, threadID);
    if (view_index < 0 || view_index > 7) {
        BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "View Index out of range 0 through 8");
        return NULL_VAL;
    }
    Scene::Views[view_index].Z = value;
    return NULL_VAL;
}
/***
 * View.SetPosition
 * \desc
 * \return
 * \ns View
 */
VMValue View_SetPosition(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_AT_LEAST_ARGCOUNT(3);
    int view_index = GetInteger(args, 0, threadID);
    if (view_index < 0 || view_index > 7) {
        BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "View Index out of range 0 through 8");
        return NULL_VAL;
    }
    Scene::Views[view_index].X = GetDecimal(args, 1, threadID);
    Scene::Views[view_index].Y = GetDecimal(args, 2, threadID);
    if (argCount == 4)
        Scene::Views[view_index].Z = GetDecimal(args, 3, threadID);
    return NULL_VAL;
}
/***
 * View.SetAngle
 * \desc
 * \return
 * \ns View
 */
VMValue View_SetAngle(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(4);
    int view_index = GetInteger(args, 0, threadID);
    if (view_index < 0 || view_index > 7) {
        BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "View Index out of range 0 through 8");
        return NULL_VAL;
    }
    Scene::Views[view_index].RotateX = GetDecimal(args, 1, threadID);
    Scene::Views[view_index].RotateY = GetDecimal(args, 2, threadID);
    Scene::Views[view_index].RotateZ = GetDecimal(args, 3, threadID);
    return NULL_VAL;
}
/***
 * View.GetX
 * \desc
 * \return
 * \ns View
 */
VMValue View_GetX(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int view_index = GetInteger(args, 0, threadID);
    if (view_index < 0 || view_index > 7) {
        BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "View Index out of range 0 through 8");
        return NULL_VAL;
    }
    return DECIMAL_VAL(Scene::Views[view_index].X);
}
/***
 * View.GetY
 * \desc
 * \return
 * \ns View
 */
VMValue View_GetY(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int view_index = GetInteger(args, 0, threadID);
    if (view_index < 0 || view_index > 7) {
        BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "View Index out of range 0 through 8");
        return NULL_VAL;
    }
    return DECIMAL_VAL(Scene::Views[GetInteger(args, 0, threadID)].Y);
}
/***
 * View.GetZ
 * \desc
 * \return
 * \ns View
 */
VMValue View_GetZ(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int view_index = GetInteger(args, 0, threadID);
    if (view_index < 0 || view_index > 7) {
        BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "View Index out of range 0 through 8");
        return NULL_VAL;
    }
    return DECIMAL_VAL(Scene::Views[GetInteger(args, 0, threadID)].Z);
}
/***
 * View.GetWidth
 * \desc
 * \return
 * \ns View
 */
VMValue View_GetWidth(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int view_index = GetInteger(args, 0, threadID);
    if (view_index < 0 || view_index > 7) {
        BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "View Index out of range 0 through 8");
        return NULL_VAL;
    }
    return DECIMAL_VAL(Scene::Views[view_index].Width);
}
/***
 * View.GetHeight
 * \desc
 * \return
 * \ns View
 */
VMValue View_GetHeight(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int view_index = GetInteger(args, 0, threadID);
    if (view_index < 0 || view_index > 7) {
        BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "View Index out of range 0 through 8");
        return NULL_VAL;
    }
    return DECIMAL_VAL(Scene::Views[view_index].Height);
}
/***
 * View.SetSize
 * \desc
 * \return
 * \ns View
 */
VMValue View_SetSize(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(3);
    int view_index = GetInteger(args, 0, threadID);
    float view_w = GetDecimal(args, 1, threadID);
    float view_h = GetDecimal(args, 2, threadID);
    if (view_index < 0 || view_index > 7) {
        BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "View Index out of range 0 through 8");
        return NULL_VAL;
    }
    Scene::Views[view_index].Width = view_w;
    Scene::Views[view_index].Height = view_h;
    return NULL_VAL;
}
/***
 * View.IsUsingDrawTarget
 * \desc
 * \return
 * \ns View
 */
VMValue View_IsUsingDrawTarget(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int view_index = GetInteger(args, 0, threadID);
    if (view_index < 0 || view_index > 7) {
        BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "View Index out of range 0 through 8");
        return NULL_VAL;
    }
    return INTEGER_VAL((int)Scene::Views[view_index].UseDrawTarget);
}
/***
 * View.SetUseDrawTarget
 * \desc
 * \return
 * \ns View
 */
VMValue View_SetUseDrawTarget(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    int view_index = GetInteger(args, 0, threadID);
    int usedrawtar = GetInteger(args, 1, threadID);
    if (view_index < 0 || view_index > 7) {
        BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "View Index out of range 0 through 8");
        return NULL_VAL;
    }
    Scene::Views[view_index].UseDrawTarget = !!usedrawtar;
    return NULL_VAL;
}
/***
 * View.IsUsingSoftwareRenderer
 * \desc
 * \return
 * \ns View
 */
VMValue View_IsUsingSoftwareRenderer(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int view_index = GetInteger(args, 0, threadID);
    if (view_index < 0 || view_index > 7) {
        BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "View Index out of range 0 through 8");
        return NULL_VAL;
    }
    return INTEGER_VAL((int)Scene::Views[view_index].Software);
}
/***
 * View.SetUseSoftwareRenderer
 * \desc
 * \return
 * \ns View
 */
VMValue View_SetUseSoftwareRenderer(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    int view_index = GetInteger(args, 0, threadID);
    int usedswrend = GetInteger(args, 1, threadID);
    if (view_index < 0 || view_index > 7) {
        BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "View Index out of range 0 through 8");
        return NULL_VAL;
    }
    Scene::Views[view_index].Software = !!usedswrend;
    return NULL_VAL;
}
/***
 * View.SetUsePerspective
 * \desc
 * \return
 * \ns View
 */
VMValue View_SetUsePerspective(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(4);
    int view_index = GetInteger(args, 0, threadID);
    int useperspec = GetInteger(args, 1, threadID);
    float nearPlane = GetDecimal(args, 2, threadID);
    float farPlane = GetDecimal(args, 3, threadID);
    if (view_index < 0 || view_index > 7) {
        BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "View Index out of range 0 through 8");
        return NULL_VAL;
    }
    Scene::Views[view_index].UsePerspective = !!useperspec;
    Scene::Views[view_index].NearPlane = nearPlane;
    Scene::Views[view_index].FarPlane = farPlane;
    return NULL_VAL;
}
/***
 * View.IsEnabled
 * \desc
 * \return
 * \ns View
 */
VMValue View_IsEnabled(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    int view_index = GetInteger(args, 0, threadID);
    if (view_index < 0 || view_index > 7) {
        BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "View Index out of range 0 through 8");
        return NULL_VAL;
    }
    return INTEGER_VAL(Scene::Views[view_index].Active);
}
/***
 * View.SetEnabled
 * \desc
 * \return
 * \ns View
 */
VMValue View_SetEnabled(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    int view_index = GetInteger(args, 0, threadID);
    int enabledddd = GetInteger(args, 1, threadID);
    if (view_index < 0 || view_index > 7) {
        BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "View Index out of range 0 through 8");
        return NULL_VAL;
    }
    Scene::Views[view_index].Active = !!enabledddd;
    return NULL_VAL;
}
/***
 * View.SetFieldOfView
 * \desc
 * \return
 * \ns View
 */
VMValue View_SetFieldOfView(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    int view_index = GetInteger(args, 0, threadID);
    float fovy = GetDecimal(args, 1, threadID);
    if (view_index < 0 || view_index > 7) {
        BytecodeObjectManager::Threads[threadID].ThrowRuntimeError(false, "View Index out of range 0 through 8");
        return NULL_VAL;
    }
    Scene::Views[view_index].FOV = fovy;
    return NULL_VAL;
}
/***
 * View.GetCurrent
 * \desc
 * \return
 * \ns View
 */
VMValue View_GetCurrent(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    return INTEGER_VAL(Scene::ViewCurrent);
}
// #endregion

// #region Window
/***
 * Window.SetSize
 * \desc
 * \return
 * \ns Window
 */
VMValue Window_SetSize(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    // HACK: Make variable "IsWindowResizeable"
    if (Application::Platform == Platforms::iOS ||
        Application::Platform == Platforms::Android)
        return NULL_VAL;

    int window_w = (int)GetDecimal(args, 0, threadID);
    int window_h = (int)GetDecimal(args, 1, threadID);
    SDL_SetWindowSize(Application::Window, window_w, window_h);

    int defaultMonitor = 0;
    Application::Settings->GetInteger("display", "defaultMonitor", &defaultMonitor);
    SDL_SetWindowPosition(Application::Window, SDL_WINDOWPOS_CENTERED_DISPLAY(defaultMonitor), SDL_WINDOWPOS_CENTERED_DISPLAY(defaultMonitor));

    // Incase the window just doesn't resize (Android)
    SDL_GetWindowSize(Application::Window, &window_w, &window_h);

    Graphics::Resize(window_w, window_h);
    return NULL_VAL;
}
/***
 * Window.SetFullscreen
 * \desc
 * \return
 * \ns Window
 */
VMValue Window_SetFullscreen(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);

    SDL_SetWindowFullscreen(Application::Window, GetInteger(args, 0, threadID) ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);

    int window_w, window_h;
    SDL_GetWindowSize(Application::Window, &window_w, &window_h);

    Graphics::Resize(window_w, window_h);
    return NULL_VAL;
}
/***
 * Window.SetBorderless
 * \desc
 * \return
 * \ns Window
 */
VMValue Window_SetBorderless(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    SDL_SetWindowBordered(Application::Window, (SDL_bool)!GetInteger(args, 0, threadID));
    return NULL_VAL;
}
/***
 * Window.SetPosition
 * \desc
 * \return
 * \ns Window
 */
VMValue Window_SetPosition(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(2);
    SDL_SetWindowPosition(Application::Window, GetInteger(args, 0, threadID), GetInteger(args, 1, threadID));
    return NULL_VAL;
}
/***
 * Window.SetPositionCentered
 * \desc
 * \return
 * \ns Window
 */
VMValue Window_SetPositionCentered(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    SDL_SetWindowPosition(Application::Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    return NULL_VAL;
}
/***
 * Window.SetTitle
 * \desc
 * \return
 * \ns Window
 */
VMValue Window_SetTitle(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(1);
    char* string = GetString(args, 0, threadID);
    memset(Application::WindowTitle, 0, sizeof(Application::WindowTitle));
    sprintf(Application::WindowTitle, "%s", string);
    SDL_SetWindowTitle(Application::Window, Application::WindowTitle);
    return NULL_VAL;
}
/***
 * Window.GetWidth
 * \desc
 * \return
 * \ns Window
 */
VMValue Window_GetWidth(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    int v; SDL_GetWindowSize(Application::Window, &v, NULL);
    return INTEGER_VAL(v);
}
/***
 * Window.GetHeight
 * \desc
 * \return
 * \ns Window
 */
VMValue Window_GetHeight(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    int v; SDL_GetWindowSize(Application::Window, NULL, &v);
    return INTEGER_VAL(v);
}
/***
 * Window.GetFullscreen
 * \desc
 * \return
 * \ns Window
 */
VMValue Window_GetFullscreen(int argCount, VMValue* args, Uint32 threadID) {
    CHECK_ARGCOUNT(0);
    int v = !!(SDL_GetWindowFlags(Application::Window) & SDL_WINDOW_FULLSCREEN_DESKTOP);
    return INTEGER_VAL(v);
}
// #endregion

#define String_CaseMapBind(lowerCase, upperCase) \
    String_ToUpperCase_Map_ExtendedASCII[(Uint8)lowerCase] = (Uint8)upperCase; \
    String_ToLowerCase_Map_ExtendedASCII[(Uint8)upperCase] = (Uint8)lowerCase;

PUBLIC STATIC void StandardLibrary::Link() {
    VMValue val;
    ObjClass* klass;

    for (int i = 0; i < 0x100; i++) {
        String_ToUpperCase_Map_ExtendedASCII[i] = (Uint8)i;
        String_ToLowerCase_Map_ExtendedASCII[i] = (Uint8)i;
    }
    for (int i = 'a'; i <= 'z'; i++) {
        int upperCase = i + ('A' - 'a');
        String_ToUpperCase_Map_ExtendedASCII[i] = (Uint8)upperCase;
        String_ToLowerCase_Map_ExtendedASCII[upperCase] = (Uint8)i;
    }
    String_CaseMapBind(L'ç', L'Ç');
    String_CaseMapBind(L'ü', L'Ü');
    String_CaseMapBind(L'á', L'Ç');
    String_CaseMapBind(L'é', L'É');
    String_CaseMapBind(L'ç', L'Ç');
    String_CaseMapBind(L'ç', L'Ç');
    String_CaseMapBind(L'ç', L'Ç');
    String_CaseMapBind(L'ç', L'Ç');
    String_CaseMapBind(L'ç', L'Ç');

    #define INIT_CLASS(className) \
        klass = NewClass(Murmur::EncryptString(#className)); \
        klass->Name = CopyString(#className, strlen(#className)); \
        val = OBJECT_VAL(klass); \
        BytecodeObjectManager::Globals->Put(klass->Hash, OBJECT_VAL(klass));
    #define DEF_NATIVE(className, funcName) \
        BytecodeObjectManager::DefineNative(klass, #funcName, className##_##funcName);

    // #region Array
    INIT_CLASS(Array);
    DEF_NATIVE(Array, Create);
    DEF_NATIVE(Array, Length);
    DEF_NATIVE(Array, Push);
    DEF_NATIVE(Array, Pop);
    DEF_NATIVE(Array, Insert);
    DEF_NATIVE(Array, Erase);
    DEF_NATIVE(Array, Clear);
    DEF_NATIVE(Array, Shift);
    // #endregion

    // #region Date
    INIT_CLASS(Date);
    DEF_NATIVE(Date, GetEpoch);
    // #endregion

    // #region Device
    INIT_CLASS(Device);
    DEF_NATIVE(Device, GetPlatform);
    DEF_NATIVE(Device, IsMobile);
    BytecodeObjectManager::GlobalConstInteger(NULL, "Platform_Windows", (int)Platforms::Windows);
    BytecodeObjectManager::GlobalConstInteger(NULL, "Platform_MacOSX", (int)Platforms::MacOSX);
    BytecodeObjectManager::GlobalConstInteger(NULL, "Platform_Linux", (int)Platforms::Linux);
    BytecodeObjectManager::GlobalConstInteger(NULL, "Platform_Ubuntu", (int)Platforms::Ubuntu);
    BytecodeObjectManager::GlobalConstInteger(NULL, "Platform_Switch", (int)Platforms::Switch);
    BytecodeObjectManager::GlobalConstInteger(NULL, "Platform_iOS", (int)Platforms::iOS);
    BytecodeObjectManager::GlobalConstInteger(NULL, "Platform_Android", (int)Platforms::Android);
    BytecodeObjectManager::GlobalConstInteger(NULL, "Platform_Default", (int)Platforms::Default);
    // #endregion

    // #region Directory
    INIT_CLASS(Directory);
    DEF_NATIVE(Directory, Create);
    DEF_NATIVE(Directory, Exists);
    DEF_NATIVE(Directory, GetFiles);
    DEF_NATIVE(Directory, GetDirectories);
    // #endregion

    // #region Display
    INIT_CLASS(Display);
    DEF_NATIVE(Display, GetWidth);
    DEF_NATIVE(Display, GetHeight);
    // #endregion

    // #region Draw
    INIT_CLASS(Draw);
    DEF_NATIVE(Draw, Sprite);
    DEF_NATIVE(Draw, SpritePart);
    DEF_NATIVE(Draw, Image);
    DEF_NATIVE(Draw, ImagePart);
    DEF_NATIVE(Draw, ImageSized);
    DEF_NATIVE(Draw, ImagePartSized);
    DEF_NATIVE(Draw, Video);
    DEF_NATIVE(Draw, VideoPart);
    DEF_NATIVE(Draw, VideoSized);
    DEF_NATIVE(Draw, VideoPartSized);
    DEF_NATIVE(Draw, Tile);
    DEF_NATIVE(Draw, Texture);
    DEF_NATIVE(Draw, TextureSized);
    DEF_NATIVE(Draw, TexturePart);
    DEF_NATIVE(Draw, SetFont);
    DEF_NATIVE(Draw, SetTextAlign);
    DEF_NATIVE(Draw, SetTextBaseline);
    DEF_NATIVE(Draw, SetTextAdvance);
    DEF_NATIVE(Draw, SetTextLineAscent);
    DEF_NATIVE(Draw, MeasureText);
    DEF_NATIVE(Draw, MeasureTextWrapped);
    DEF_NATIVE(Draw, Text);
    DEF_NATIVE(Draw, TextWrapped);
    DEF_NATIVE(Draw, TextEllipsis);
    DEF_NATIVE(Draw, SetBlendColor);
    DEF_NATIVE(Draw, SetTextureBlend);
    DEF_NATIVE(Draw, SetBlendMode);
    DEF_NATIVE(Draw, SetBlendFactor);
    DEF_NATIVE(Draw, SetBlendFactorExtended);
    DEF_NATIVE(Draw, SetCompareColor);
    DEF_NATIVE(Draw, Line);
    DEF_NATIVE(Draw, Circle);
    DEF_NATIVE(Draw, Ellipse);
    DEF_NATIVE(Draw, Triangle);
    DEF_NATIVE(Draw, Rectangle);
    DEF_NATIVE(Draw, CircleStroke);
    DEF_NATIVE(Draw, EllipseStroke);
    DEF_NATIVE(Draw, TriangleStroke);
    DEF_NATIVE(Draw, RectangleStroke);
    DEF_NATIVE(Draw, UseFillSmoothing);
    DEF_NATIVE(Draw, UseStrokeSmoothing);
    DEF_NATIVE(Draw, SetClip);
    DEF_NATIVE(Draw, ClearClip);
    DEF_NATIVE(Draw, Save);
    DEF_NATIVE(Draw, Scale);
    DEF_NATIVE(Draw, Rotate);
    DEF_NATIVE(Draw, Restore);
    DEF_NATIVE(Draw, Translate);
    DEF_NATIVE(Draw, SetTextureTarget);
    DEF_NATIVE(Draw, Clear);
    DEF_NATIVE(Draw, ResetTextureTarget);

    BytecodeObjectManager::GlobalConstInteger(NULL, "BlendMode_ADD", BlendMode_ADD);
    BytecodeObjectManager::GlobalConstInteger(NULL, "BlendMode_MAX", BlendMode_MAX);
    BytecodeObjectManager::GlobalConstInteger(NULL, "BlendMode_NORMAL", BlendMode_NORMAL);
    BytecodeObjectManager::GlobalConstInteger(NULL, "BlendMode_SUBTRACT", BlendMode_SUBTRACT);
    BytecodeObjectManager::GlobalConstInteger(NULL, "BlendMode_MATCH_EQUAL", BlendMode_MATCH_EQUAL);
    BytecodeObjectManager::GlobalConstInteger(NULL, "BlendMode_MATCH_NOT_EQUAL", BlendMode_MATCH_NOT_EQUAL);

    BytecodeObjectManager::GlobalConstInteger(NULL, "BlendFactor_ZERO", BlendFactor_ZERO);
    BytecodeObjectManager::GlobalConstInteger(NULL, "BlendFactor_ONE", BlendFactor_ONE);
    BytecodeObjectManager::GlobalConstInteger(NULL, "BlendFactor_SRC_COLOR", BlendFactor_SRC_COLOR);
    BytecodeObjectManager::GlobalConstInteger(NULL, "BlendFactor_INV_SRC_COLOR", BlendFactor_INV_SRC_COLOR);
    BytecodeObjectManager::GlobalConstInteger(NULL, "BlendFactor_SRC_ALPHA", BlendFactor_SRC_ALPHA);
    BytecodeObjectManager::GlobalConstInteger(NULL, "BlendFactor_INV_SRC_ALPHA", BlendFactor_INV_SRC_ALPHA);
    BytecodeObjectManager::GlobalConstInteger(NULL, "BlendFactor_DST_COLOR", BlendFactor_DST_COLOR);
    BytecodeObjectManager::GlobalConstInteger(NULL, "BlendFactor_INV_DST_COLOR", BlendFactor_INV_DST_COLOR);
    BytecodeObjectManager::GlobalConstInteger(NULL, "BlendFactor_DST_ALPHA", BlendFactor_DST_ALPHA);
    BytecodeObjectManager::GlobalConstInteger(NULL, "BlendFactor_INV_DST_ALPHA", BlendFactor_INV_DST_ALPHA);
    // #endregion

    // #region Ease
    INIT_CLASS(Ease);
    DEF_NATIVE(Ease, InSine);
    DEF_NATIVE(Ease, OutSine);
    DEF_NATIVE(Ease, InOutSine);
    DEF_NATIVE(Ease, InQuad);
    DEF_NATIVE(Ease, OutQuad);
    DEF_NATIVE(Ease, InOutQuad);
    DEF_NATIVE(Ease, InCubic);
    DEF_NATIVE(Ease, OutCubic);
    DEF_NATIVE(Ease, InOutCubic);
    DEF_NATIVE(Ease, InQuart);
    DEF_NATIVE(Ease, OutQuart);
    DEF_NATIVE(Ease, InOutQuart);
    DEF_NATIVE(Ease, InQuint);
    DEF_NATIVE(Ease, OutQuint);
    DEF_NATIVE(Ease, InOutQuint);
    DEF_NATIVE(Ease, InExpo);
    DEF_NATIVE(Ease, OutExpo);
    DEF_NATIVE(Ease, InOutExpo);
    DEF_NATIVE(Ease, InCirc);
    DEF_NATIVE(Ease, OutCirc);
    DEF_NATIVE(Ease, InOutCirc);
    DEF_NATIVE(Ease, InBack);
    DEF_NATIVE(Ease, OutBack);
    DEF_NATIVE(Ease, InOutBack);
    DEF_NATIVE(Ease, InElastic);
    DEF_NATIVE(Ease, OutElastic);
    DEF_NATIVE(Ease, InOutElastic);
    DEF_NATIVE(Ease, InBounce);
    DEF_NATIVE(Ease, OutBounce);
    DEF_NATIVE(Ease, InOutBounce);
    DEF_NATIVE(Ease, Triangle);
    // #endregion

    // #region File
    INIT_CLASS(File);
    DEF_NATIVE(File, Exists);
    DEF_NATIVE(File, ReadAllText);
    DEF_NATIVE(File, WriteAllText);
    // #endregion

    // #region HTTP
    INIT_CLASS(HTTP);
    DEF_NATIVE(HTTP, GetString);
    DEF_NATIVE(HTTP, GetToFile);
    // #endregion

    // #region Input
    INIT_CLASS(Input);
    DEF_NATIVE(Input, GetMouseX);
    DEF_NATIVE(Input, GetMouseY);
    DEF_NATIVE(Input, IsMouseButtonDown);
    DEF_NATIVE(Input, IsMouseButtonPressed);
    DEF_NATIVE(Input, IsMouseButtonReleased);
    DEF_NATIVE(Input, IsKeyDown);
    DEF_NATIVE(Input, IsKeyPressed);
    DEF_NATIVE(Input, IsKeyReleased);
    DEF_NATIVE(Input, GetControllerAttached);
    DEF_NATIVE(Input, GetControllerHat);
    DEF_NATIVE(Input, GetControllerAxis);
    DEF_NATIVE(Input, GetControllerButton);
    DEF_NATIVE(Input, GetControllerName);
    // DEF_NATIVE(Key, IsDown);
    // DEF_NATIVE(Key, IsPressed);
    // DEF_NATIVE(Key, IsReleased);
    // DEF_NATIVE(Controller, IsAttached);
    // DEF_NATIVE(Controller, GetHat);
    // DEF_NATIVE(Controller, GetAxis);
    // DEF_NATIVE(Controller, GetButton);
    // DEF_NATIVE(Controller, GetName);
    // DEF_NATIVE(Mouse, GetX);
    // DEF_NATIVE(Mouse, GetY);
    // DEF_NATIVE(Mouse, IsButtonDown);
    // DEF_NATIVE(Mouse, IsButtonPressed);
    // DEF_NATIVE(Mouse, IsButtonReleased);
    // #endregion

    // #region Instance
    INIT_CLASS(Instance);
    DEF_NATIVE(Instance, Create);
    DEF_NATIVE(Instance, GetNth);
    DEF_NATIVE(Instance, GetCount);
    DEF_NATIVE(Instance, GetNextInstance);
    // #endregion

    // #region JSON
    INIT_CLASS(JSON);
    DEF_NATIVE(JSON, Parse);
    DEF_NATIVE(JSON, ToString);
    // #endregion

    // #region Math
    INIT_CLASS(Math);
    DEF_NATIVE(Math, Cos);
    DEF_NATIVE(Math, Sin);
    DEF_NATIVE(Math, Acos);
    DEF_NATIVE(Math, Asin);
    DEF_NATIVE(Math, Atan);
    DEF_NATIVE(Math, Distance);
    DEF_NATIVE(Math, Direction);
    DEF_NATIVE(Math, Abs);
    DEF_NATIVE(Math, Min);
    DEF_NATIVE(Math, Max);
    DEF_NATIVE(Math, Clamp);
    DEF_NATIVE(Math, Sign);
    DEF_NATIVE(Math, Random);
    DEF_NATIVE(Math, RandomMax);
    DEF_NATIVE(Math, RandomRange);
    DEF_NATIVE(Math, Floor);
    DEF_NATIVE(Math, Ceil);
    DEF_NATIVE(Math, Round);
    DEF_NATIVE(Math, Sqrt);
    DEF_NATIVE(Math, Pow);
    DEF_NATIVE(Math, Exp);
    // #endregion

    // #region Music
    INIT_CLASS(Music);
    DEF_NATIVE(Music, Play);
    DEF_NATIVE(Music, Stop);
    DEF_NATIVE(Music, StopWithFadeOut);
    DEF_NATIVE(Music, Pause);
    DEF_NATIVE(Music, Resume);
    DEF_NATIVE(Music, Clear);
    DEF_NATIVE(Music, Loop);
    DEF_NATIVE(Music, IsPlaying);
    // #endregion

    // #region Number
    INIT_CLASS(Number);
    DEF_NATIVE(Number, ToString);
    DEF_NATIVE(Number, AsInteger);
    DEF_NATIVE(Number, AsDecimal);
    // #endregion

    // #region Palette
    INIT_CLASS(Palette);
    DEF_NATIVE(Palette, EnablePaletteUsage);
    DEF_NATIVE(Palette, LoadFromFile);
    DEF_NATIVE(Palette, LoadFromResource);
    DEF_NATIVE(Palette, GetColor);
    DEF_NATIVE(Palette, SetColor);
    DEF_NATIVE(Palette, MixPalettes);
    DEF_NATIVE(Palette, RotateColorsLeft);
    DEF_NATIVE(Palette, RotateColorsRight);
    DEF_NATIVE(Palette, CopyColors);
    DEF_NATIVE(Palette, SetPaletteIndexLines);
    // #endregion

    // #region Resources
    INIT_CLASS(Resources);
    DEF_NATIVE(Resources, LoadSprite);
    DEF_NATIVE(Resources, LoadImage);
    DEF_NATIVE(Resources, LoadFont);
    DEF_NATIVE(Resources, LoadShader);
    DEF_NATIVE(Resources, LoadModel);
    DEF_NATIVE(Resources, LoadMusic);
    DEF_NATIVE(Resources, LoadSound);
    DEF_NATIVE(Resources, LoadVideo);
    DEF_NATIVE(Resources, FileExists);

    DEF_NATIVE(Resources, UnloadImage);

    BytecodeObjectManager::GlobalConstInteger(NULL, "SCOPE_SCENE", 0);
    BytecodeObjectManager::GlobalConstInteger(NULL, "SCOPE_GAME", 1);
    // #endregion

    // #region Scene
    INIT_CLASS(Scene);
    DEF_NATIVE(Scene, Load);
    DEF_NATIVE(Scene, LoadTileCollisions);
    DEF_NATIVE(Scene, Restart);
    DEF_NATIVE(Scene, GetLayerCount);
    DEF_NATIVE(Scene, GetLayerIndex);
    DEF_NATIVE(Scene, GetName);
    DEF_NATIVE(Scene, GetWidth);
    DEF_NATIVE(Scene, GetHeight);
    DEF_NATIVE(Scene, GetTileSize);
    DEF_NATIVE(Scene, GetTileID);
    DEF_NATIVE(Scene, GetTileFlipX);
    DEF_NATIVE(Scene, GetTileFlipY);
    DEF_NATIVE(Scene, SetTile);
    DEF_NATIVE(Scene, SetTileCollisionSides);
    DEF_NATIVE(Scene, SetPaused);
    DEF_NATIVE(Scene, SetLayerVisible);
    DEF_NATIVE(Scene, SetLayerOffsetPosition);
    DEF_NATIVE(Scene, SetLayerDrawGroup);
    DEF_NATIVE(Scene, IsPaused);
    // #endregion

    // #region Shader
    INIT_CLASS(Shader);
    DEF_NATIVE(Shader, Set);
    DEF_NATIVE(Shader, GetUniform);
    DEF_NATIVE(Shader, SetUniformI);
    DEF_NATIVE(Shader, SetUniformF);
    DEF_NATIVE(Shader, SetUniform3x3);
    DEF_NATIVE(Shader, SetUniform4x4);
    DEF_NATIVE(Shader, SetUniformTexture);
    DEF_NATIVE(Shader, Unset);
    // #endregion

    // #region SocketClient
    INIT_CLASS(SocketClient);
    DEF_NATIVE(SocketClient, Open);
    DEF_NATIVE(SocketClient, Close);
    DEF_NATIVE(SocketClient, IsOpen);
    DEF_NATIVE(SocketClient, Poll);
    DEF_NATIVE(SocketClient, BytesToRead);
    DEF_NATIVE(SocketClient, ReadDecimal);
    DEF_NATIVE(SocketClient, ReadInteger);
    DEF_NATIVE(SocketClient, ReadString);
    DEF_NATIVE(SocketClient, WriteDecimal);
    DEF_NATIVE(SocketClient, WriteInteger);
    DEF_NATIVE(SocketClient, WriteString);
    // #endregion

    // #region Sound
    INIT_CLASS(Sound);
    DEF_NATIVE(Sound, Play);
    DEF_NATIVE(Sound, Loop);
    DEF_NATIVE(Sound, Stop);
    DEF_NATIVE(Sound, Pause);
    DEF_NATIVE(Sound, Resume);
    DEF_NATIVE(Sound, StopAll);
    DEF_NATIVE(Sound, PauseAll);
    DEF_NATIVE(Sound, ResumeAll);
    // #endregion

    // #region Sprite
    INIT_CLASS(Sprite);
    DEF_NATIVE(Sprite, GetAnimationCount);
    DEF_NATIVE(Sprite, GetFrameLoopIndex);
    DEF_NATIVE(Sprite, GetFrameCount);
    DEF_NATIVE(Sprite, GetFrameDuration);
    DEF_NATIVE(Sprite, GetFrameSpeed);
    // #endregion

    // #region String
    INIT_CLASS(String);
    DEF_NATIVE(String, Split);
    DEF_NATIVE(String, CharAt);
    DEF_NATIVE(String, Length);
    DEF_NATIVE(String, Compare);
    DEF_NATIVE(String, IndexOf);
    DEF_NATIVE(String, Contains);
    DEF_NATIVE(String, Substring);
    DEF_NATIVE(String, ToUpperCase);
    DEF_NATIVE(String, ToLowerCase);
    DEF_NATIVE(String, LastIndexOf);
    DEF_NATIVE(String, ParseInteger);
    DEF_NATIVE(String, ParseDecimal);
    // #endregion

    // #region Texture
    INIT_CLASS(Texture);
    DEF_NATIVE(Texture, FromSprite);
    DEF_NATIVE(Texture, FromImage);
    DEF_NATIVE(Texture, Create);
    DEF_NATIVE(Texture, SetInterpolation);
    // #endregion

    // #region Touch
    INIT_CLASS(Touch);
    DEF_NATIVE(Touch, GetX);
    DEF_NATIVE(Touch, GetY);
    DEF_NATIVE(Touch, IsDown);
    DEF_NATIVE(Touch, IsPressed);
    DEF_NATIVE(Touch, IsReleased);
    // #endregion

    // #region TileCollision
    INIT_CLASS(TileCollision);
    DEF_NATIVE(TileCollision, Point);
    DEF_NATIVE(TileCollision, PointExtended);
    // #endregion

    // #region TileInfo
    INIT_CLASS(TileInfo);
    DEF_NATIVE(TileInfo, IsEmptySpace);
    // #endregion

    // #region Thread
    INIT_CLASS(Thread);
    DEF_NATIVE(Thread, RunEvent);
    DEF_NATIVE(Thread, Sleep);
    // #endregion

    // #region Video
    INIT_CLASS(Video);
    DEF_NATIVE(Video, Play);
    DEF_NATIVE(Video, Pause);
    DEF_NATIVE(Video, Resume);
    DEF_NATIVE(Video, Stop);
    DEF_NATIVE(Video, Close);
    DEF_NATIVE(Video, IsBuffering);
    DEF_NATIVE(Video, IsPlaying);
    DEF_NATIVE(Video, IsPaused);
    DEF_NATIVE(Video, SetPosition);
    DEF_NATIVE(Video, SetBufferDuration);
    DEF_NATIVE(Video, SetTrackEnabled);
    DEF_NATIVE(Video, GetPosition);
    DEF_NATIVE(Video, GetDuration);
    DEF_NATIVE(Video, GetBufferDuration);
    DEF_NATIVE(Video, GetBufferEnd);
    DEF_NATIVE(Video, GetTrackCount);
    DEF_NATIVE(Video, GetTrackEnabled);
    DEF_NATIVE(Video, GetTrackName);
    DEF_NATIVE(Video, GetTrackLanguage);
    DEF_NATIVE(Video, GetDefaultVideoTrack);
    DEF_NATIVE(Video, GetDefaultAudioTrack);
    DEF_NATIVE(Video, GetDefaultSubtitleTrack);
    DEF_NATIVE(Video, GetWidth);
    DEF_NATIVE(Video, GetHeight);
    // #endregion

    // #region View
    INIT_CLASS(View);
    DEF_NATIVE(View, SetX);
    DEF_NATIVE(View, SetY);
    DEF_NATIVE(View, SetZ);
    DEF_NATIVE(View, SetPosition);
    DEF_NATIVE(View, SetAngle);
    DEF_NATIVE(View, SetSize);
    DEF_NATIVE(View, GetX);
    DEF_NATIVE(View, GetY);
    DEF_NATIVE(View, GetZ);
    DEF_NATIVE(View, GetWidth);
    DEF_NATIVE(View, GetHeight);
    DEF_NATIVE(View, IsUsingDrawTarget);
    DEF_NATIVE(View, SetUseDrawTarget);
    DEF_NATIVE(View, SetUsePerspective);
    DEF_NATIVE(View, IsUsingSoftwareRenderer);
    DEF_NATIVE(View, SetUseSoftwareRenderer);
    DEF_NATIVE(View, IsEnabled);
    DEF_NATIVE(View, SetEnabled);
    DEF_NATIVE(View, SetFieldOfView);
    DEF_NATIVE(View, GetCurrent);
    // #endregion

    // #region Window
    INIT_CLASS(Window);
    DEF_NATIVE(Window, SetSize);
    DEF_NATIVE(Window, SetFullscreen);
    DEF_NATIVE(Window, SetBorderless);
    DEF_NATIVE(Window, SetPosition);
    DEF_NATIVE(Window, SetPositionCentered);
    DEF_NATIVE(Window, SetTitle);
    DEF_NATIVE(Window, GetWidth);
    DEF_NATIVE(Window, GetHeight);
    DEF_NATIVE(Window, GetFullscreen);
    // #endregion

    #undef DEF_NATIVE
    #undef INIT_CLASS

    BytecodeObjectManager::Globals->Put("other", NULL_VAL);

    BytecodeObjectManager::GlobalLinkDecimal(NULL, "CameraX", &Scene::Views[0].X);
    BytecodeObjectManager::GlobalLinkDecimal(NULL, "CameraY", &Scene::Views[0].Y);
    BytecodeObjectManager::GlobalLinkDecimal(NULL, "LowPassFilter", &AudioManager::LowPassFilter);

    BytecodeObjectManager::GlobalLinkInteger(NULL, "Scene_Frame", &Scene::Frame);

    BytecodeObjectManager::GlobalConstDecimal(NULL, "Math_PI", M_PI);
    BytecodeObjectManager::GlobalConstDecimal(NULL, "Math_PI_DOUBLE", M_PI * 2.0);
    BytecodeObjectManager::GlobalConstDecimal(NULL, "Math_PI_HALF", M_PI / 2.0);

    #define CONST_KEY(key) BytecodeObjectManager::GlobalConstInteger(NULL, "Key_"#key, SDL_SCANCODE_##key);
    {
        CONST_KEY(A);
        CONST_KEY(B);
        CONST_KEY(C);
        CONST_KEY(D);
        CONST_KEY(E);
        CONST_KEY(F);
        CONST_KEY(G);
        CONST_KEY(H);
        CONST_KEY(I);
        CONST_KEY(J);
        CONST_KEY(K);
        CONST_KEY(L);
        CONST_KEY(M);
        CONST_KEY(N);
        CONST_KEY(O);
        CONST_KEY(P);
        CONST_KEY(Q);
        CONST_KEY(R);
        CONST_KEY(S);
        CONST_KEY(T);
        CONST_KEY(U);
        CONST_KEY(V);
        CONST_KEY(W);
        CONST_KEY(X);
        CONST_KEY(Y);
        CONST_KEY(Z);

        CONST_KEY(1);
        CONST_KEY(2);
        CONST_KEY(3);
        CONST_KEY(4);
        CONST_KEY(5);
        CONST_KEY(6);
        CONST_KEY(7);
        CONST_KEY(8);
        CONST_KEY(9);
        CONST_KEY(0);

        CONST_KEY(RETURN);
        CONST_KEY(ESCAPE);
        CONST_KEY(BACKSPACE);
        CONST_KEY(TAB);
        CONST_KEY(SPACE);

        CONST_KEY(MINUS);
        CONST_KEY(EQUALS);
        CONST_KEY(LEFTBRACKET);
        CONST_KEY(RIGHTBRACKET);
        CONST_KEY(BACKSLASH);
        CONST_KEY(SEMICOLON);
        CONST_KEY(APOSTROPHE);
        CONST_KEY(GRAVE);
        CONST_KEY(COMMA);
        CONST_KEY(PERIOD);
        CONST_KEY(SLASH);

        CONST_KEY(CAPSLOCK);

        CONST_KEY(SEMICOLON);

        CONST_KEY(F1);
        CONST_KEY(F2);
        CONST_KEY(F3);
        CONST_KEY(F4);
        CONST_KEY(F5);
        CONST_KEY(F6);
        CONST_KEY(F7);
        CONST_KEY(F8);
        CONST_KEY(F9);
        CONST_KEY(F10);
        CONST_KEY(F11);
        CONST_KEY(F12);

        CONST_KEY(PRINTSCREEN);
        CONST_KEY(SCROLLLOCK);
        CONST_KEY(PAUSE);
        CONST_KEY(INSERT);
        CONST_KEY(HOME);
        CONST_KEY(PAGEUP);
        CONST_KEY(DELETE);
        CONST_KEY(END);
        CONST_KEY(PAGEDOWN);
        CONST_KEY(RIGHT);
        CONST_KEY(LEFT);
        CONST_KEY(DOWN);
        CONST_KEY(UP);

        CONST_KEY(NUMLOCKCLEAR);
        CONST_KEY(KP_DIVIDE);
        CONST_KEY(KP_MULTIPLY);
        CONST_KEY(KP_MINUS);
        CONST_KEY(KP_PLUS);
        CONST_KEY(KP_ENTER);
        CONST_KEY(KP_1);
        CONST_KEY(KP_2);
        CONST_KEY(KP_3);
        CONST_KEY(KP_4);
        CONST_KEY(KP_5);
        CONST_KEY(KP_6);
        CONST_KEY(KP_7);
        CONST_KEY(KP_8);
        CONST_KEY(KP_9);
        CONST_KEY(KP_0);
        CONST_KEY(KP_PERIOD);

        CONST_KEY(LCTRL);
        CONST_KEY(LSHIFT);
        CONST_KEY(LALT);
        CONST_KEY(LGUI);
        CONST_KEY(RCTRL);
        CONST_KEY(RSHIFT);
        CONST_KEY(RALT);
        CONST_KEY(RGUI);
    }
    #undef  CONST_KEY
}
PUBLIC STATIC void StandardLibrary::Dispose() {

}
