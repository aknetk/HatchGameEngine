#ifndef ENGINE_RENDERING_D3D_D3DRENDERER_H
#define ENGINE_RENDERING_D3D_D3DRENDERER_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED


#include <Engine/Includes/Standard.h>
#include <Engine/Includes/StandardSDL2.h>
#include <Engine/ResourceTypes/ISprite.h>
#include <Engine/Rendering/Texture.h>
#include <Engine/Includes/HashMap.h>
#include <stack>

class D3DRenderer {
public:
    static void     Init();
    static Uint32   GetWindowFlags();
    static void     SetGraphicsFunctions();
    static void     Dispose();
    static Texture* CreateTexture(Uint32 format, Uint32 access, Uint32 width, Uint32 height);
    static int      LockTexture(Texture* texture, void** pixels, int* pitch);
    static int      UpdateTexture(Texture* texture, SDL_Rect* r, void* pixels, int pitch);
    static void     UnlockTexture(Texture* texture);
    static void     DisposeTexture(Texture* texture);
    static void     SetRenderTarget(Texture* texture);
    static void     UpdateWindowSize(int width, int height);
    static void     UpdateViewport();
    static void     UpdateClipRect();
    static void     UpdateOrtho(float left, float top, float right, float bottom);
    static void     UpdatePerspective(float fovy, float aspect, float nearv, float farv);
    static void     UpdateProjectionMatrix();
    static void     UseShader(void* shader);
    static void     SetUniformF(int location, int count, float* values);
    static void     SetUniformI(int location, int count, int* values);
    static void     SetUniformTexture(Texture* texture, int uniform_index, int slot);
    static void     Clear();
    static void     Present();
    static void     SetBlendColor(float r, float g, float b, float a);
    static void     SetBlendMode(int srcC, int dstC, int srcA, int dstA);
    static void     SetLineWidth(float n);
    static void     StrokeLine(float x1, float y1, float x2, float y2);
    static void     StrokeCircle(float x, float y, float rad);
    static void     StrokeEllipse(float x, float y, float w, float h);
    static void     StrokeRectangle(float x, float y, float w, float h);
    static void     FillCircle(float x, float y, float rad);
    static void     FillEllipse(float x, float y, float w, float h);
    static void     FillTriangle(float x1, float y1, float x2, float y2, float x3, float y3);
    static void     FillRectangle(float x, float y, float w, float h);
    static void     DrawTexture(Texture* texture, float sx, float sy, float sw, float sh, float x, float y, float w, float h);
    static void     DrawSprite(ISprite* sprite, int animation, int frame, int x, int y, bool flipX, bool flipY, float scaleW, float scaleH, float rotation);
    static void     DrawSpritePart(ISprite* sprite, int animation, int frame, int sx, int sy, int sw, int sh, int x, int y, bool flipX, bool flipY, float scaleW, float scaleH, float rotation);
    static Uint32   CreateTexturedShapeBuffer(float** data, int vertexCount);
    static void     DrawTexturedShapeBuffer(Texture* texture, Uint32 bufferID, int vertexCount);
};

#endif /* ENGINE_RENDERING_D3D_D3DRENDERER_H */
