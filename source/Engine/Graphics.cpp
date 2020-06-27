#if INTERFACE
#include <Engine/Includes/Standard.h>
#include <Engine/Includes/StandardSDL2.h>

#include <Engine/Application.h>
#include <Engine/Math/Matrix4x4.h>
#include <Engine/ResourceTypes/ISprite.h>
#include <Engine/ResourceTypes/IModel.h>
#include <Engine/Includes/HashMap.h>
#include <Engine/Rendering/Enums.h>

need_t ISprite;
need_t IModel;

class Graphics {
public:
	static HashMap<Texture*>* TextureMap;
	static HashMap<Texture*>* SpriteSheetTextureMap;
	static bool   		      VsyncEnabled;
	static int 				  MultisamplingEnabled;
	static int 				  FontDPI;
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
    static bool               SmoothFill;
    static bool               SmoothStroke;

	// Rendering functions
	static GraphicsFunctions  Internal;
	static GraphicsFunctions* GfxFunctions;
	static const char*        Renderer;
    static float              PixelOffset;
	static bool 			  NoInternalTextures;
	static int   			  BlendMode;
};
#endif

#include <Engine/Graphics.h>

#include <Engine/Diagnostics/Log.h>
#include <Engine/Diagnostics/Memory.h>
#include <Engine/Math/Math.h>
#include <Engine/Rendering/Software/SoftwareRenderer.h>
#include <Engine/Rendering/D3D/D3DRenderer.h>
#include <Engine/Rendering/GL/GLRenderer.h>

HashMap<Texture*>* Graphics::TextureMap = NULL;
HashMap<Texture*>* Graphics::SpriteSheetTextureMap = NULL;
bool   		  	   Graphics::VsyncEnabled = true;
int   		  	   Graphics::MultisamplingEnabled = 0;
int 			   Graphics::FontDPI = 1;
bool   		  	   Graphics::SupportsBatching = false;
bool               Graphics::TextureBlend = false;
bool               Graphics::TextureInterpolate = false;
Uint32             Graphics::PreferredPixelFormat = SDL_PIXELFORMAT_ARGB8888;
Uint32 		       Graphics::MaxTextureWidth = 1;
Uint32 		       Graphics::MaxTextureHeight = 1;
Texture*           Graphics::TextureHead = NULL;

stack<Matrix4x4*>  Graphics::ProjectionMatrixBackups;
stack<Matrix4x4*>  Graphics::ModelViewMatrix;

Viewport           Graphics::CurrentViewport;
Viewport           Graphics::BackupViewport;
ClipArea           Graphics::CurrentClip;
ClipArea           Graphics::BackupClip;

float              Graphics::BlendColors[4];

Texture*           Graphics::CurrentRenderTarget = NULL;

void*              Graphics::CurrentShader = NULL;
bool               Graphics::SmoothFill = false;
bool               Graphics::SmoothStroke = false;

GraphicsFunctions  Graphics::Internal;
GraphicsFunctions* Graphics::GfxFunctions = &Graphics::Internal;
const char*        Graphics::Renderer = "default";
float              Graphics::PixelOffset = 0.0f;
bool               Graphics::NoInternalTextures = false;
int   			   Graphics::BlendMode = 0;

/*
// Rendering functions
void     (*Graphics::Internal_Init)() = NULL;
Uint32   (*Graphics::Internal_GetWindowFlags)() = NULL;
void     (*Graphics::Internal_SetGraphicsFunctions)() = NULL;
void     (*Graphics::Internal_Dispose)() = NULL;

// Texture management functions
Texture* (*Graphics::Internal_CreateTexture)(Uint32, Uint32, Uint32, Uint32) = NULL;
Texture* (*Graphics::Internal_CreateTextureFromPixels)(Uint32, Uint32, void*, int) = NULL;
Texture* (*Graphics::Internal_CreateTextureFromSurface)(SDL_Surface*) = NULL;
int      (*Graphics::Internal_LockTexture)(Texture*, void**, int*) = NULL;
int      (*Graphics::Internal_UpdateTexture)(Texture*, SDL_Rect*, void*, int) = NULL;
int      (*Graphics::Internal_UpdateYUVTexture)(Texture*, SDL_Rect*, void*, int, void*, int, void*, int) = NULL;
void     (*Graphics::Internal_UnlockTexture)(Texture*) = NULL;
void     (*Graphics::Internal_DisposeTexture)(Texture*) = NULL;

// Viewport and view-related functions
void     (*Graphics::Internal_SetRenderTarget)(Texture* texture) = NULL;
void     (*Graphics::Internal_UpdateWindowSize)(int width, int height) = NULL;
void     (*Graphics::Internal_UpdateViewport)() = NULL;
void     (*Graphics::Internal_UpdateClipRect)() = NULL;
void     (*Graphics::Internal_UpdateOrtho)(float, float, float width, float height) = NULL;
void     (*Graphics::Internal_UpdatePerspective)(float fovy, float aspect, float near, float far) = NULL;
void     (*Graphics::Internal_UpdateProjectionMatrix)() = NULL;

// Shader-related functions
void     (*Graphics::Internal_UseShader)(void* shader) = NULL;
void     (*Graphics::Internal_SetTextureInterpolation)(bool interpolate) = NULL;
void     (*Graphics::Internal_SetUniformTexture)(Texture* texture, int uniform_index, int slot) = NULL;
void     (*Graphics::Internal_SetUniformF)(int location, int count, float* values) = NULL;
void     (*Graphics::Internal_SetUniformI)(int location, int count, int* values) = NULL;

// These guys
void     (*Graphics::Internal_Clear)() = NULL;
void     (*Graphics::Internal_Present)() = NULL;

// Draw mode setting functions
void     (*Graphics::Internal_SetBlendColor)(float r, float g, float b, float a) = NULL;
void     (*Graphics::Internal_SetBlendMode)(int srcC, int dstC, int srcA, int dstA) = NULL;
void     (*Graphics::Internal_SetLineWidth)(float n) = NULL;

// Primitive drawing functions
void     (*Graphics::Internal_StrokeLine)(float x1, float y1, float x2, float y2) = NULL;
void     (*Graphics::Internal_StrokeCircle)(float x, float y, float rad) = NULL;
void     (*Graphics::Internal_StrokeEllipse)(float x, float y, float w, float h) = NULL;
void     (*Graphics::Internal_StrokeRectangle)(float x, float y, float w, float h) = NULL;
void     (*Graphics::Internal_FillCircle)(float x, float y, float rad) = NULL;
void     (*Graphics::Internal_FillEllipse)(float x, float y, float w, float h) = NULL;
void     (*Graphics::Internal_FillTriangle)(float x1, float y1, float x2, float y2, float x3, float y3) = NULL;
void     (*Graphics::Internal_FillRectangle)(float x, float y, float w, float h) = NULL;

// Texture drawing functions
void     (*Graphics::Internal_DrawTexture)(Texture* texture, float sx, float sy, float sw, float sh, float x, float y, float w, float h) = NULL;
void     (*Graphics::Internal_DrawSprite)(ISprite* sprite, int animation, int frame, int x, int y, bool flipX, bool flipY) = NULL;
void     (*Graphics::Internal_DrawSpritePart)(ISprite* sprite, int animation, int frame, int sx, int sy, int sw, int sh, int x, int y, bool flipX, bool flipY) = NULL;

void     (*Graphics::Internal_MakeFrameBufferID)(ISprite* sprite, AnimFrame* frame) = NULL;

//*/

PUBLIC STATIC void     Graphics::Init() {
	Graphics::TextureMap = new HashMap<Texture*>(NULL, 32);
    Graphics::SpriteSheetTextureMap = new HashMap<Texture*>(NULL, 32);

    Graphics::ModelViewMatrix.push(Matrix4x4::Create());

	Graphics::CurrentClip.Enabled = false;
	Graphics::BackupClip.Enabled = false;

	Graphics::BlendColors[0] =
	Graphics::BlendColors[1] =
	Graphics::BlendColors[2] =
	Graphics::BlendColors[3] = 1.0;

	int w, h;
	SDL_GetWindowSize(Application::Window, &w, &h);

	Viewport* vp = &Graphics::CurrentViewport;
	vp->X =
	vp->Y = 0.0f;
	vp->Width = w;
	vp->Height = h;
	Graphics::BackupViewport = Graphics::CurrentViewport;

	Graphics::GfxFunctions->Init();

    Log::Print(Log::LOG_INFO, "CPU Core Count: %d", SDL_GetCPUCount());
	Log::Print(Log::LOG_INFO, "System Memory: %d MB", SDL_GetSystemRAM());
    Log::Print(Log::LOG_INFO, "Window Size: %d x %d", w, h);
    Log::Print(Log::LOG_INFO, "Window Pixel Format: %s", SDL_GetPixelFormatName(SDL_GetWindowPixelFormat(Application::Window)));
    Log::Print(Log::LOG_INFO, "VSync: %s", Graphics::VsyncEnabled ? "true" : "false");
    if (Graphics::MultisamplingEnabled)
        Log::Print(Log::LOG_INFO, "Multisample: %d", Graphics::MultisamplingEnabled);
    else
        Log::Print(Log::LOG_INFO, "Multisample: false");
    Log::Print(Log::LOG_INFO, "Max Texture Size: %d x %d", Graphics::MaxTextureWidth, Graphics::MaxTextureHeight);
}
PUBLIC STATIC void     Graphics::ChooseBackend() {
	char renderer[64];

	SoftwareRenderer::SetGraphicsFunctions();

	// Platform-default renderers
	if (Application::Settings->GetString("dev", "renderer", renderer)) {
		if (!strcmp(renderer, "opengl")) {
            Graphics::Renderer = "opengl";
			GLRenderer::SetGraphicsFunctions();
			return;
		}
		else if (!strcmp(renderer, "direct3d")) {
			#ifdef WIN32
                Graphics::Renderer = "direct3d";
				D3DRenderer::SetGraphicsFunctions();
				return;
			#else
				Log::Print(Log::LOG_ERROR, "Direct3D renderer does not exist on this platform.");
			#endif
		}
	}

	switch (Application::Platform) {
        case Platforms::Windows: {
            #ifdef WIN32
                Graphics::Renderer = "direct3d";
                D3DRenderer::SetGraphicsFunctions();
                break;
            #endif

            Graphics::Renderer = "opengl";
            GLRenderer::SetGraphicsFunctions();
            break;
        }
        case Platforms::MacOSX:
        case Platforms::Linux:
        case Platforms::Ubuntu:
            Graphics::Renderer = "opengl";
            GLRenderer::SetGraphicsFunctions();
            break;
        case Platforms::Switch:
        case Platforms::iOS:
        case Platforms::Android:
            Graphics::Renderer = "opengl";
            GLRenderer::SetGraphicsFunctions();
            break;
        default:
            Graphics::Renderer = "opengl";
            GLRenderer::SetGraphicsFunctions();
            break;
    }
}
PUBLIC STATIC Uint32   Graphics::GetWindowFlags() {
    return Graphics::GfxFunctions->GetWindowFlags();
}
PUBLIC STATIC void     Graphics::Dispose() {
	for (Texture* texture = Graphics::TextureHead, *next; texture != NULL; texture = next) {
		next = texture->Next;
        Graphics::DisposeTexture(texture);
    }
	Graphics::TextureHead = NULL;

    Graphics::SpriteSheetTextureMap->Clear();

    Graphics::GfxFunctions->Dispose();

	delete Graphics::TextureMap;
    delete Graphics::SpriteSheetTextureMap;
    while (Graphics::ModelViewMatrix.size()) {
        delete Graphics::ModelViewMatrix.top();
        Graphics::ModelViewMatrix.pop();
    }
}

PUBLIC STATIC Point    Graphics::ProjectToScreen(float x, float y, float z) {
	Point p;
	float vec4[4];
	Matrix4x4* matrix;

	// matrix = Matrix4x4::Create();
	matrix = Graphics::ModelViewMatrix.top();
	// Matrix4x4::Multiply(matrix, Graphics::ProjectionMatrix, Graphics::ModelViewMatrix.top());

	vec4[0] = x, vec4[1] = y, vec4[2] = z, vec4[3] = 1.0f;
	Matrix4x4::Multiply(matrix, &vec4[0]);
	// Matrix4x4::Multiply(Graphics::ProjectionMatrix, &vec4[0]);

	p.X = (vec4[0] + 1.0f) / 2.0f * Graphics::CurrentViewport.Width;
	p.Y = (vec4[1] - 1.0f) / -2.0f * Graphics::CurrentViewport.Height;
	// p.X = vec4[0];
	// p.Y = vec4[1];
	p.Z = vec4[2];
	return p;
}

PUBLIC STATIC Texture* Graphics::CreateTexture(Uint32 format, Uint32 access, Uint32 width, Uint32 height) {
    Texture* texture;
    if (Graphics::GfxFunctions == &SoftwareRenderer::BackendFunctions) {
        texture = Texture::New(format, access, width, height);
        if (!texture)
            return NULL;
    }
    else {
		if (!Graphics::NoInternalTextures) {
	    	if (width > Graphics::MaxTextureWidth || height > Graphics::MaxTextureHeight) {
	    		Log::Print(Log::LOG_ERROR, "Texture of size %u x %u is larger than maximum size of %u x %u!", width, height, Graphics::MaxTextureWidth, Graphics::MaxTextureHeight);
	    		return NULL;
	    	}
	        texture = Graphics::GfxFunctions->CreateTexture(format, access, width, height);
	    	if (!texture)
	    		return NULL;
		}
		else {
			texture = Texture::New(format, access, width, height);
	        if (!texture)
	            return NULL;
		}
    }

	texture->Next = Graphics::TextureHead;
	if (Graphics::TextureHead)
		Graphics::TextureHead->Prev = texture;

	Graphics::TextureHead = texture;

	return texture;
}
PUBLIC STATIC Texture* Graphics::CreateTextureFromPixels(Uint32 width, Uint32 height, void* pixels, int pitch) {
    Texture* texture = Graphics::CreateTexture(SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, width, height);
	Graphics::UpdateTexture(texture, NULL, pixels, pitch);

	return texture;
}
PUBLIC STATIC Texture* Graphics::CreateTextureFromSurface(SDL_Surface* surface) {
	Texture* texture = Graphics::CreateTexture(surface->format->format, SDL_TEXTUREACCESS_STATIC, surface->w, surface->h);
	Graphics::GfxFunctions->UpdateTexture(texture, NULL, surface->pixels, surface->pitch);
	return texture;
}
PUBLIC STATIC int      Graphics::LockTexture(Texture* texture, void** pixels, int* pitch) {
    return Graphics::GfxFunctions->LockTexture(texture, pixels, pitch);
}
PUBLIC STATIC int      Graphics::UpdateTexture(Texture* texture, SDL_Rect* src, void* pixels, int pitch) {
	memcpy(texture->Pixels, pixels, sizeof(Uint32) * texture->Width * texture->Height);

	if (!Graphics::NoInternalTextures)
		return Graphics::GfxFunctions->UpdateTexture(texture, src, pixels, pitch);

	return 0;
}
PUBLIC STATIC int      Graphics::UpdateYUVTexture(Texture* texture, SDL_Rect* src, Uint8* pixelsY, int pitchY, Uint8* pixelsU, int pitchU, Uint8* pixelsV, int pitchV) {
	if (!Graphics::GfxFunctions->UpdateYUVTexture)
		return 0;
    return Graphics::GfxFunctions->UpdateYUVTexture(texture, src, pixelsY, pitchY, pixelsU, pitchU, pixelsV, pitchV);
}
PUBLIC STATIC void     Graphics::UnlockTexture(Texture* texture) {
    Graphics::GfxFunctions->UnlockTexture(texture);
}
PUBLIC STATIC void     Graphics::DisposeTexture(Texture* texture) {
    Graphics::GfxFunctions->DisposeTexture(texture);

	if (texture->Next)
		texture->Next->Prev = texture->Prev;

	if (texture->Prev)
		texture->Prev->Next = texture->Next;
	else
		Graphics::TextureHead = texture->Next;

	Memory::Free(texture);
}

PUBLIC STATIC void     Graphics::UseShader(void* shader) {
	Graphics::CurrentShader = shader;
	if (shader)
    	Graphics::GfxFunctions->UseShader(shader);
}
PUBLIC STATIC void     Graphics::SetTextureInterpolation(bool interpolate) {
    Graphics::TextureInterpolate = interpolate;
}

PUBLIC STATIC void     Graphics::Clear() {
    Graphics::GfxFunctions->Clear();
}
PUBLIC STATIC void     Graphics::Present() {
    Graphics::GfxFunctions->Present();
}

PUBLIC STATIC void     Graphics::SoftwareStart() {
	Graphics::GfxFunctions = &SoftwareRenderer::BackendFunctions;
}
PUBLIC STATIC void     Graphics::SoftwareEnd() {
	// Present to current view texture
	// Scene::Views[Scene::ViewCurrent]
	Graphics::GfxFunctions = &Graphics::Internal;
	Graphics::UpdateTexture(Graphics::CurrentRenderTarget, NULL, Graphics::CurrentRenderTarget->Pixels, Graphics::CurrentRenderTarget->Width * 4);
}

PUBLIC STATIC void     Graphics::SetRenderTarget(Texture* texture) {
	if (texture && !Graphics::CurrentRenderTarget) {
		Graphics::BackupViewport = Graphics::CurrentViewport;
		Graphics::BackupClip = Graphics::CurrentClip;
	}

	// if (Graphics::CurrentRenderTarget != texture) {
    // 	Graphics::ProjectionMatrixBackups.push(Matrix4x4::Create());
	// 	Matrix4x4::Copy(Graphics::ProjectionMatrixBackups.top(), Scene::Views[Scene::ViewCurrent].ProjectionMatrix);
	// }
	// else {
	// 	Matrix4x4::Copy(Scene::Views[Scene::ViewCurrent].ProjectionMatrix, Graphics::ProjectionMatrixBackups.top());
	// 	delete Graphics::ProjectionMatrixBackups.top();
	// 	Graphics::ProjectionMatrixBackups.pop();
	// }

	Graphics::CurrentRenderTarget = texture;

    Graphics::GfxFunctions->SetRenderTarget(texture);

	Viewport* vp = &Graphics::CurrentViewport;
	if (texture) {
		vp->X = 0.0;
		vp->Y = 0.0;
		vp->Width = texture->Width;
		vp->Height = texture->Height;
	}
	else {
		Graphics::CurrentViewport = Graphics::BackupViewport;
		Graphics::CurrentClip = Graphics::BackupClip;
	}

    Graphics::GfxFunctions->UpdateViewport();
	Graphics::GfxFunctions->UpdateClipRect();
}
PUBLIC STATIC void     Graphics::UpdateOrtho(float width, float height) {
    Graphics::GfxFunctions->UpdateOrtho(0.0f, 0.0f, width, height);
}
PUBLIC STATIC void     Graphics::UpdateOrthoFlipped(float width, float height) {
    Graphics::GfxFunctions->UpdateOrtho(0.0f, height, width, 0.0f);
}
PUBLIC STATIC void     Graphics::UpdatePerspective(float fovy, float aspect, float nearv, float farv) {
    Graphics::GfxFunctions->UpdatePerspective(fovy, aspect, nearv, farv);
}
PUBLIC STATIC void     Graphics::UpdateProjectionMatrix() {
    Graphics::GfxFunctions->UpdateProjectionMatrix();
}
PUBLIC STATIC void     Graphics::SetViewport(float x, float y, float w, float h) {
	Viewport* vp = &Graphics::CurrentViewport;
	if (x < 0) {
		vp->X = 0.0;
		vp->Y = 0.0;
		if (Graphics::CurrentRenderTarget) {
			vp->Width = Graphics::CurrentRenderTarget->Width;
			vp->Height = Graphics::CurrentRenderTarget->Height;
		}
		else {
			int w, h;
			SDL_GetWindowSize(Application::Window, &w, &h);
			vp->Width = w;
			vp->Height = h;
		}
	}
	else {
		vp->X = x;
		vp->Y = y;
		vp->Width = w;
		vp->Height = h;
	}
    Graphics::GfxFunctions->UpdateViewport();
}
PUBLIC STATIC void     Graphics::ResetViewport() {
	Graphics::SetViewport(-1.0, 0.0, 0.0, 0.0);
}
PUBLIC STATIC void     Graphics::Resize(int width, int height) {
    Graphics::GfxFunctions->UpdateWindowSize(width, height);
}

PUBLIC STATIC void     Graphics::SetClip(int x, int y, int width, int height) {
	Graphics::CurrentClip.Enabled = true;
	Graphics::CurrentClip.X = x;
	Graphics::CurrentClip.Y = y;
	Graphics::CurrentClip.Width = width;
	Graphics::CurrentClip.Height = height;
	Graphics::GfxFunctions->UpdateClipRect();
}
PUBLIC STATIC void     Graphics::ClearClip() {
	Graphics::CurrentClip.Enabled = false;
	Graphics::GfxFunctions->UpdateClipRect();
}

PUBLIC STATIC void     Graphics::Save() {
	Matrix4x4* top = ModelViewMatrix.top();
    Matrix4x4* push = Matrix4x4::Create();
    Matrix4x4::Copy(push, top);

    if (ModelViewMatrix.size() == 256) {
		Log::Print(Log::LOG_ERROR, "Draw.Save stack too big.");
		exit(-1);
	}

    ModelViewMatrix.push(push);
}
PUBLIC STATIC void     Graphics::Translate(float x, float y, float z) {
	Matrix4x4::Translate(ModelViewMatrix.top(), ModelViewMatrix.top(), x, y, z);
}
PUBLIC STATIC void     Graphics::Rotate(float x, float y, float z) {
	Matrix4x4::Rotate(ModelViewMatrix.top(), ModelViewMatrix.top(), x, 1.0, 0.0, 0.0);
    Matrix4x4::Rotate(ModelViewMatrix.top(), ModelViewMatrix.top(), y, 0.0, 1.0, 0.0);
    Matrix4x4::Rotate(ModelViewMatrix.top(), ModelViewMatrix.top(), z, 0.0, 0.0, 1.0);
}
PUBLIC STATIC void     Graphics::Scale(float x, float y, float z) {
    Matrix4x4::Scale(ModelViewMatrix.top(), ModelViewMatrix.top(), x, y, z);
}
PUBLIC STATIC void     Graphics::Restore() {
	if (ModelViewMatrix.size() == 1) return;

    delete ModelViewMatrix.top();
    ModelViewMatrix.pop();
}

PUBLIC STATIC void     Graphics::SetBlendColor(float r, float g, float b, float a) {
	Graphics::BlendColors[0] = r;
	Graphics::BlendColors[1] = g;
	Graphics::BlendColors[2] = b;
	Graphics::BlendColors[3] = a;
    Graphics::GfxFunctions->SetBlendColor(r, g, b, a);
}
PUBLIC STATIC void     Graphics::SetBlendMode(int srcC, int dstC, int srcA, int dstA) {
    Graphics::GfxFunctions->SetBlendMode(srcC, dstC, srcA, dstA);
}
PUBLIC STATIC void     Graphics::SetLineWidth(float n) {
    Graphics::GfxFunctions->SetLineWidth(n);
}

PUBLIC STATIC void     Graphics::StrokeLine(float x1, float y1, float x2, float y2) {
    Graphics::GfxFunctions->StrokeLine(x1, y1, x2, y2);
}
PUBLIC STATIC void     Graphics::StrokeCircle(float x, float y, float rad) {
    Graphics::GfxFunctions->StrokeCircle(x, y, rad);
}
PUBLIC STATIC void     Graphics::StrokeEllipse(float x, float y, float w, float h) {
    Graphics::GfxFunctions->StrokeEllipse(x, y, w, h);
}
PUBLIC STATIC void     Graphics::StrokeTriangle(float x1, float y1, float x2, float y2, float x3, float y3) {
	Graphics::GfxFunctions->StrokeLine(x1, y1, x2, y2);
	Graphics::GfxFunctions->StrokeLine(x2, y2, x3, y3);
	Graphics::GfxFunctions->StrokeLine(x3, y3, x1, y1);
}
PUBLIC STATIC void     Graphics::StrokeRectangle(float x, float y, float w, float h) {
    Graphics::GfxFunctions->StrokeRectangle(x, y, w, h);
}
PUBLIC STATIC void     Graphics::FillCircle(float x, float y, float rad) {
    Graphics::GfxFunctions->FillCircle(x, y, rad);
}
PUBLIC STATIC void     Graphics::FillEllipse(float x, float y, float w, float h) {
    Graphics::GfxFunctions->FillEllipse(x, y, w, h);
}
PUBLIC STATIC void     Graphics::FillTriangle(float x1, float y1, float x2, float y2, float x3, float y3) {
	Graphics::GfxFunctions->FillTriangle(x1, y1, x2, y2, x3, y3);
}
PUBLIC STATIC void     Graphics::FillRectangle(float x, float y, float w, float h) {
    Graphics::GfxFunctions->FillRectangle(x, y, w, h);
}

PUBLIC STATIC void     Graphics::DrawTexture(Texture* texture, float sx, float sy, float sw, float sh, float x, float y, float w, float h) {
    Graphics::GfxFunctions->DrawTexture(texture, sx, sy, sw, sh, x, y, w, h);
}
PUBLIC STATIC void     Graphics::DrawSprite(ISprite* sprite, int animation, int frame, int x, int y, bool flipX, bool flipY, float scaleW, float scaleH, float rotation) {
    Graphics::GfxFunctions->DrawSprite(sprite, animation, frame, x, y, flipX, flipY, scaleW, scaleH, rotation);
}
PUBLIC STATIC void     Graphics::DrawSpritePart(ISprite* sprite, int animation, int frame, int sx, int sy, int sw, int sh, int x, int y, bool flipX, bool flipY, float scaleW, float scaleH, float rotation) {
    Graphics::GfxFunctions->DrawSpritePart(sprite, animation, frame, sx, sy, sw, sh, x, y, flipX, flipY, scaleW, scaleH, rotation);
}

PUBLIC STATIC void     Graphics::MakeFrameBufferID(ISprite* sprite, AnimFrame* frame) {
    if (Graphics::GfxFunctions->MakeFrameBufferID)
        Graphics::GfxFunctions->MakeFrameBufferID(sprite, frame);
}

PUBLIC STATIC bool     Graphics::SpriteRangeCheck(ISprite* sprite, int animation, int frame) {
	#ifdef DEBUG
	if (!sprite) return true;
	if (animation < 0 || animation >= (int)sprite->Animations.size()) {
		Log::Print(Log::LOG_ERROR, "Animation %d does not exist in sprite %s!", animation, sprite->Filename);
		assert(animation >= 0 && animation < (int)sprite->Animations.size());
		return true;
	}
	if (frame < 0 || frame >= (int)sprite->Animations[animation].Frames.size()) {
		Log::Print(Log::LOG_ERROR, "Frame %d in animation \"%s\" does not exist in sprite %s!", frame, sprite->Animations[animation].Name, sprite->Filename);
		assert(frame >= 0 && frame < (int)sprite->Animations[animation].Frames.size());
		return true;
	}
	#endif
	return false;
}
