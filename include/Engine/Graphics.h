#ifndef GRAPHICS_H
#define GRAPHICS_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL

class ISprite;
class IModel;

#include <Engine/Includes/Standard.h>
#include <Engine/Includes/StandardSDL2.h>
#include <Engine/Application.h>
#include <Engine/Math/Matrix4x4.h>
#include <Engine/Rendering/Enums.h>
#include <Engine/ResourceTypes/ISprite.h>
#include <Engine/ResourceTypes/IModel.h>
#include <Engine/Includes/HashMap.h>

class Graphics {
public:
    static HashMap<Texture*>* TextureMap;
    static HashMap<Texture*>* SpriteSheetTextureMap;
    static bool   		      VsyncEnabled;
    static int 				  MultisamplingEnabled;
    static bool   		      SupportsBatching;
    static bool   		      TextureBlend;
    static bool   		      TextureInterpolate;
    static Uint32             PreferredPixelFormat;
    static Uint32 		      MaxTextureWidth;
    static Uint32 		      MaxTextureHeight;
    static Texture*           TextureHead;
    static stack<Matrix4x4*>  ProjectionMatrixBackups;
    static stack<Matrix4x4*>  ModelViewMatrix;
    static Viewport           CurrentViewport;
    static Viewport           BackupViewport;
    static ClipArea           CurrentClip;
    static ClipArea           BackupClip;
    static float              BlendColors[4];
    static Texture*           CurrentRenderTarget;
    static void*              CurrentShader;
    // Rendering functions
    static void     (*Internal_Init)();
    static Uint32   (*Internal_GetWindowFlags)();
    static void     (*Internal_SetGraphicsFunctions)();
    static void     (*Internal_Dispose)();
    static Texture* (*Internal_CreateTexture)(Uint32 format, Uint32 access, Uint32 width, Uint32 height);
    static Texture* (*Internal_CreateTextureFromPixels)(Uint32 width, Uint32 height, void* pixels, int pitch);
    static Texture* (*Internal_CreateTextureFromSurface)(SDL_Surface* surface);
    static int      (*Internal_LockTexture)(Texture* texture, void** pixels, int* pitch);
    static int      (*Internal_UpdateTexture)(Texture* texture, SDL_Rect* src, void* pixels, int pitch);
    static int      (*Internal_UpdateYUVTexture)(Texture* texture, SDL_Rect* src, void* pixelsY, int pitchY, void* pixelsU, int pitchU, void* pixelsV, int pitchV);
    static void     (*Internal_UnlockTexture)(Texture* texture);
    static void     (*Internal_DisposeTexture)(Texture* texture);
    static void     (*Internal_UseShader)(void* shader);
    static void     (*Internal_SetTextureInterpolation)(bool interpolate);
    static void     (*Internal_SetUniformTexture)(Texture* texture, int uniform_index, int slot);
    static void     (*Internal_SetUniformF)(int location, int count, float* values);
    static void     (*Internal_SetUniformI)(int location, int count, int* values);
    static void     (*Internal_UpdateViewport)();
    static void     (*Internal_UpdateClipRect)();
    static void     (*Internal_UpdateOrtho)(float left, float top, float right, float bottom);
    static void     (*Internal_UpdatePerspective)(float fovy, float aspect, float near, float far);
    static void     (*Internal_UpdateProjectionMatrix)();
    static void     (*Internal_Clear)();
    static void     (*Internal_Present)();
    static void     (*Internal_SetRenderTarget)(Texture* texture);
    static void     (*Internal_UpdateWindowSize)(int width, int height);
    static void     (*Internal_SetBlendColor)(float r, float g, float b, float a);
    static void     (*Internal_SetBlendMode)(int srcC, int dstC, int srcA, int dstA);
    static void     (*Internal_SetLineWidth)(float n);
    static void     (*Internal_StrokeLine)(float x1, float y1, float x2, float y2);
    static void     (*Internal_StrokeCircle)(float x, float y, float rad);
    static void     (*Internal_StrokeEllipse)(float x, float y, float w, float h);
    static void     (*Internal_StrokeRectangle)(float x, float y, float w, float h);
    static void     (*Internal_FillCircle)(float x, float y, float rad);
    static void     (*Internal_FillEllipse)(float x, float y, float w, float h);
    static void     (*Internal_FillTriangle)(float x1, float y1, float x2, float y2, float x3, float y3);
    static void     (*Internal_FillRectangle)(float x, float y, float w, float h);
    static void     (*Internal_DrawTexture)(Texture* texture, float sx, float sy, float sw, float sh, float x, float y, float w, float h);
    static void     (*Internal_DrawSprite)(ISprite* sprite, int animation, int frame, int x, int y, bool flipX, bool flipY);

    static void     Init();
    static void     ChooseBackend();
    static Uint32   GetWindowFlags();
    static void     Dispose();
    static Point    ProjectToScreen(float x, float y, float z);
    static Texture* CreateTexture(Uint32 format, Uint32 access, Uint32 width, Uint32 height);
    static Texture* CreateTextureFromPixels(Uint32 width, Uint32 height, void* pixels, int pitch);
    static Texture* CreateTextureFromSurface(SDL_Surface* surface);
    static int      LockTexture(Texture* texture, void** pixels, int* pitch);
    static int      UpdateTexture(Texture* texture, SDL_Rect* src, void* pixels, int pitch);
    static int      UpdateYUVTexture(Texture* texture, SDL_Rect* src, Uint8* pixelsY, int pitchY, Uint8* pixelsU, int pitchU, Uint8* pixelsV, int pitchV);
    static void     UnlockTexture(Texture* texture);
    static void     DisposeTexture(Texture* texture);
    static void     UseShader(void* shader);
    static void     SetTextureInterpolation(bool interpolate);
    static void     Clear();
    static void     Present();
    static void     SetRenderTarget(Texture* texture);
    static void     UpdateOrtho(float width, float height);
    static void     UpdateOrthoFlipped(float width, float height);
    static void     UpdatePerspective(float fovy, float aspect, float nearv, float farv);
    static void     UpdateProjectionMatrix();
    static void     SetViewport(float x, float y, float w, float h);
    static void     ResetViewport();
    static void     Resize(int width, int height);
    static void     SetClip(int x, int y, int width, int height);
    static void     ClearClip();
    static void     Save();
    static void     Translate(float x, float y, float z);
    static void     Rotate(float x, float y, float z);
    static void     Scale(float x, float y, float z);
    static void     Restore();
    static void     SetBlendColor(float r, float g, float b, float a);
    static void     SetBlendMode(int srcC, int dstC, int srcA, int dstA);
    static void     SetLineWidth(float n);
    static void     StrokeLine(float x1, float y1, float x2, float y2);
    static void     StrokeCircle(float x, float y, float rad);
    static void     StrokeEllipse(float x, float y, float w, float h);
    static void     StrokeTriangle(float x1, float y1, float x2, float y2, float x3, float y3);
    static void     StrokeRectangle(float x, float y, float w, float h);
    static void     FillCircle(float x, float y, float rad);
    static void     FillEllipse(float x, float y, float w, float h);
    static void     FillTriangle(float x1, float y1, float x2, float y2, float x3, float y3);
    static void     FillRectangle(float x, float y, float w, float h);
    static void     DrawTexture(Texture* texture, float sx, float sy, float sw, float sh, float x, float y, float w, float h);
    static void     DrawSprite(ISprite* sprite, int animation, int frame, int x, int y, bool flipX, bool flipY);
    static bool     SpriteRangeCheck(ISprite* sprite, int animation, int frame);
};

#endif /* GRAPHICS_H */
