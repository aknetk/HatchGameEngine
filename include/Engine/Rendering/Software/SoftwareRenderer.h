#ifndef ENGINE_RENDERING_SOFTWARE_SOFTWARERENDERER_H
#define ENGINE_RENDERING_SOFTWARE_SOFTWARERENDERER_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED


#include <Engine/Includes/Standard.h>
#include <Engine/Includes/StandardSDL2.h>
#include <Engine/ResourceTypes/ISprite.h>
#include <Engine/ResourceTypes/IModel.h>
#include <Engine/Math/Matrix4x4.h>
#include <Engine/Rendering/GL/GLShader.h>
#include <Engine/Rendering/Texture.h>
#include <Engine/Includes/HashMap.h>

class SoftwareRenderer {
public:
    static GraphicsFunctions  BackendFunctions;

    static inline Uint32 GetPixelIndex(ISprite* sprite, int x, int y);
    static void    SetDrawAlpha(int a);
    static void    SetDrawFunc(int a);
    static void    SetFade(int fade);
    static void    SetFilter(int filter);
    static int     GetFilter();
    static bool    IsOnScreen(int x, int y, int w, int h);
    static void    DrawPoint(int x, int y, Uint32 color);
    static void    DrawLine(int x0, int y0, int x1, int y1, Uint32 color);
    static void    DrawCircle(int x, int y, int radius, Uint32 color);
    static void    DrawCircleStroke(int x, int y, int radius, Uint32 color);
    static void    DrawEllipse(int x, int y, int width, int height, Uint32 color);
    static void    DrawEllipseStroke(int x, int y, int width, int height, int radius, Uint32 color);
    static void    DrawTriangle(int p0_x, int p0_y, int p1_x, int p1_y, int p2_x, int p2_y, Uint32 color);
    static void    DrawTriangleStroke(int p0_x, int p0_y, int p1_x, int p1_y, int p2_x, int p2_y, Uint32 color);
    static void    DrawRectangle(int x, int y, int width, int height, Uint32 color);
    static void    DrawRectangleStroke(int x, int y, int width, int height, Uint32 color);
    static void    DrawRectangleSkewedH(int x, int y, int width, int height, int sk, Uint32 color);
    static void    DrawSpriteNormal(ISprite* sprite, int SrcX, int SrcY, int Width, int Height, int CenterX, int CenterY, bool FlipX, bool FlipY, int RealCenterX, int RealCenterY);
    static void    DrawSpriteTransformed(ISprite* sprite, int animation, int frame, int x, int y, bool flipX, bool flipY, int scaleW, int scaleH, int angle);
    static void    DrawSpriteSizedTransformed(ISprite* sprite, int animation, int frame, int x, int y, bool flipX, bool flipY, int width, int height, int angle);
    static void    DrawSpriteAtlas(ISprite* sprite, int x, int y, int pivotX, int pivotY, bool flipX, bool flipY);
    static void    DrawSpritePart(ISprite* sprite, int animation, int frame, int srcx, int srcy, int srcw, int srch, int x, int y, bool flipX, bool flipY);
    static void    DrawText(int x, int y, const char* string, unsigned int pixel);
    static void    DrawTextShadow(int x, int y, const char* string, unsigned int pixel);
    static void    DrawTextSprite(ISprite* sprite, int animation, char first, int x, int y, const char* string);
    static int     MeasureTextSprite(ISprite* sprite, int animation, char first, const char* string);
    static void    DrawModelOn2D(IModel* model, int x, int y, double scale, int rx, int ry, int rz, Uint32 color, bool wireframe);
    static void    DrawSpriteIn3D(ISprite* sprite, int animation, int frame, int x, int y, int z, double scale, int rx, int ry, int rz);
    static Uint32  ColorAdd(Uint32 color1, Uint32 color2, int percent);
    static Uint32  ColorBlend(Uint32 color1, Uint32 color2, int percent);
    static void    ScanLine(long x1, long y1, long x2, long y2);
    static void     Init();
    static Uint32   GetWindowFlags();
    static void     SetGraphicsFunctions(void* baseGFXfunctions);
    static void     Dispose();
    static Texture* CreateTexture(Uint32 format, Uint32 access, Uint32 width, Uint32 height);
    static int      LockTexture(Texture* texture, void** pixels, int* pitch);
    static int      UpdateTexture(Texture* texture, SDL_Rect* src, void* pixels, int pitch);
    static int      UpdateYUVTexture(Texture* texture, SDL_Rect* src, Uint8* pixelsY, int pitchY, Uint8* pixelsU, int pitchU, Uint8* pixelsV, int pitchV);
    static void     UnlockTexture(Texture* texture);
    static void     DisposeTexture(Texture* texture);
    static void     SetRenderTarget(Texture* texture);
    static void     UseShader(void* shader);
    static void     SetUniformF(int location, int count, float* values);
    static void     SetUniformI(int location, int count, int* values);
    static void     SetUniformTexture(Texture* texture, int uniform_index, int slot);
    static void     Clear();
    static void     Present();
    static void     SetBlendColor(float r, float g, float b, float a);
    static void     SetBlendMode(int srcC, int dstC, int srcA, int dstA);
    static void     Resize(int width, int height);
    static void     SetClip(float x, float y, float width, float height);
    static void     ClearClip();
    static void     Save();
    static void     Translate(float x, float y, float z);
    static void     Rotate(float x, float y, float z);
    static void     Scale(float x, float y, float z);
    static void     Restore();
    static void     SetLineWidth(float n);
    static void     StrokeLine(float x1, float y1, float x2, float y2);
    static void     StrokeCircle(float x, float y, float rad);
    static void     StrokeEllipse(float x, float y, float w, float h);
    static void     StrokeRectangle(float x, float y, float w, float h);
    static void     FillCircle(float x, float y, float rad);
    static void     FillEllipse(float x, float y, float w, float h);
    static void     FillRectangle(float x, float y, float w, float h);
    static void     DrawTexture(Texture* texture, float sx, float sy, float sw, float sh, float x, float y, float w, float h);
    static void     DrawTexture(Texture* texture, float x, float y, float w, float h);
    static void     DrawTexture(Texture* texture, float x, float y);
};

#endif /* ENGINE_RENDERING_SOFTWARE_SOFTWARERENDERER_H */
