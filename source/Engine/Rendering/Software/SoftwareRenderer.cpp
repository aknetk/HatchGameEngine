#if INTERFACE
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
    static GraphicsFunctions BackendFunctions;
    static Uint32            CompareColor;
    static Sint32            CurrentPalette;
    static Uint32            PaletteColors[32][0x100];
    static Uint8             PaletteIndexLines[4096];
};
#endif

#include <Engine/Rendering/Software/SoftwareRenderer.h>

#include <Engine/Diagnostics/Log.h>
#include <Engine/Diagnostics/Memory.h>
#include <Engine/IO/ResourceStream.h>

#include <Engine/Bytecode/Types.h>

GraphicsFunctions SoftwareRenderer::BackendFunctions;
Uint32            SoftwareRenderer::CompareColor = 0xFF000000U;
Sint32            SoftwareRenderer::CurrentPalette = -1;
Uint32            SoftwareRenderer::PaletteColors[32][0x100];
Uint8             SoftwareRenderer::PaletteIndexLines[4096];

int Alpha = 0xFF;
int BlendFlag = 0;
int DivideBy3Table[0x100];

struct SWTextureData {
	Uint32*           Palette = NULL;
	int               PaletteSize = 0;
	int               PaletteCount = 0;
	int               TransparentColorIndex = 0;
};

inline Uint32 ColorAdd(Uint32 color1, Uint32 color2, int percent) {
	Uint32 r = (color1 & 0xFF0000U) + (((color2 & 0xFF0000U) * percent) >> 8);
	Uint32 g = (color1 & 0xFF00U) + (((color2 & 0xFF00U) * percent) >> 8);
	Uint32 b = (color1 & 0xFFU) + (((color2 & 0xFFU) * percent) >> 8);
    if (r > 0xFF0000U) r = 0xFF0000U;
	if (g > 0xFF00U) g = 0xFF00U;
	if (b > 0xFFU) b = 0xFFU;
	return r | g | b;
}
inline Uint32 ColorBlend(Uint32 color1, Uint32 color2, int percent) {
	Uint32 rb = color1 & 0xFF00FFU;
	Uint32 g  = color1 & 0x00FF00U;
	rb += (((color2 & 0xFF00FFU) - rb) * percent) >> 8;
	g  += (((color2 & 0x00FF00U) - g) * percent) >> 8;
	return (rb & 0xFF00FFU) | (g & 0x00FF00U);
}
inline Uint32 ColorSubtract(Uint32 color1, Uint32 color2, int percent) {
    Sint32 r = (color1 & 0xFF0000U) - (((color2 & 0xFF0000U) * percent) >> 8);
	Sint32 g = (color1 & 0xFF00U) - (((color2 & 0xFF00U) * percent) >> 8);
	Sint32 b = (color1 & 0xFFU) - (((color2 & 0xFFU) * percent) >> 8);
    if (r < 0) r = 0;
	if (g < 0) g = 0;
	if (b < 0) b = 0;
	return r | g | b;
}

inline void SetPixel(int x, int y, Uint32 color) {
	// if (DoDeform) x += Deform[y];

	// if (x <  Clip[0]) return;
	// if (y <  Clip[1]) return;
	// if (x >= Clip[2]) return;
	// if (y >= Clip[3]) return;

	// SetPixelFunction(x, y, GetPixelOutputColor(color));
	// SetPixelFunction(x, y, color);
}

inline Uint32 GetPixelOutputColor(Uint32 color) {
	// color = (this->*SetFilterFunction[0])(color);
	// color = (this->*SetFilterFunction[1])(color);
	// color = (this->*SetFilterFunction[2])(color);
	// color = (this->*SetFilterFunction[3])(color);

	return color;
}

bool SaveScreenshot(const char* filename) {
    /*
	Uint32* palette = (Uint32*)Memory::Calloc(256, sizeof(Uint32));
	Uint32* pixelChecked = (Uint32*)Memory::Calloc(FrameBufferSize, sizeof(Uint32));
	Uint32* pixelIndexes = (Uint32*)Memory::Calloc(FrameBufferSize, sizeof(Uint32));
	Uint32  paletteCount = 0;
	for (int checking = 0; checking < FrameBufferSize; checking++) {
		if (!pixelChecked[checking]) {
			Uint32 color = FrameBuffer[checking] & 0xFFFFFF;
			for (int comparing = checking; comparing < FrameBufferSize; comparing++) {
				if (color == (FrameBuffer[comparing] & 0xFFFFFF)) {
					pixelIndexes[comparing] = paletteCount;
					pixelChecked[comparing] = 1;
				}
			}
			if (paletteCount == 255) {
				Memory::Free(palette);
				Memory::Free(pixelChecked);
				Memory::Free(pixelIndexes);
				Log::Print(Log::LOG_ERROR, "Too many colors!");
				return false;
			}
			palette[paletteCount++] = color;
		}
	}

	char filenameStr[100];
	time_t now; time(&now);
	struct tm* local = localtime(&now);
	sprintf(filenameStr, "ImpostorEngine3_%02d-%02d-%02d_%02d-%02d-%02d.gif", local->tm_mday, local->tm_mon + 1, local->tm_year - 100, local->tm_hour, local->tm_min, local->tm_sec);

	GIF* gif = new GIF;
	gif->Colors = palette;
    gif->Data = pixelIndexes;
    gif->Width = Application::WIDTH;
    gif->Height = Application::HEIGHT;
    gif->TransparentColorIndex = 0xFF;
	if (!gif->Save(filenameStr)) {
		gif->Data = NULL;
		delete gif;
		Memory::Free(palette);
		Memory::Free(pixelChecked);
		Memory::Free(pixelIndexes);
		return false;
	}
	gif->Data = NULL;
	delete gif;
	Memory::Free(palette);
	Memory::Free(pixelChecked);
	Memory::Free(pixelIndexes);

    */

	return true;
}

PUBLIC STATIC inline Uint32 SoftwareRenderer::GetPixelIndex(ISprite* sprite, int x, int y) {
	// return sprite->Data[x + y * sprite->Width];
	return 0;
}

// Rendering functions
PUBLIC STATIC void    SoftwareRenderer::SetDrawAlpha(int a) {
	// DrawAlpha = a;
    //
	// if (SetPixelFunction != &SoftwareRenderer::SetPixelNormal && SetPixelFunction != &SoftwareRenderer::SetPixelAlpha) return;
    //
	// if (DrawAlpha == 0xFF)
	// 	SetPixelFunction = &SoftwareRenderer::SetPixelNormal;
	// else
	// 	SetPixelFunction = &SoftwareRenderer::SetPixelAlpha;
}
PUBLIC STATIC void    SoftwareRenderer::SetDrawFunc(int a) {
	// switch (a) {
	// 	case 1:
	// 		SetPixelFunction = &SoftwareRenderer::SetPixelAdditive;
	// 		break;
	// 	default:
	// 		SetPixelFunction = &SoftwareRenderer::SetPixelNormal;
	// 		SetDrawAlpha(DrawAlpha);
	// 		break;
	// }
}

PUBLIC STATIC void    SoftwareRenderer::SetFade(int fade) {
	// if (fade < 0)
	// 	Fade = 0x00;
	// else if (fade > 0xFF)
	// 	Fade = 0xFF;
	// else
	// 	Fade = fade;
}
PUBLIC STATIC void    SoftwareRenderer::SetFilter(int filter) {
	// DrawPixelFilter = filter;
    //
	// SetFilterFunction[0] = &SoftwareRenderer::FilterNone;
	// // SetFilterFunction[1] = &SoftwareRenderer::FilterNone;
	// // SetFilterFunction[2] = &SoftwareRenderer::FilterNone;
	// SetFilterFunction[3] = &SoftwareRenderer::FilterNone;
	// if ((DrawPixelFilter & 0x1) == 0x1)
	// 	SetFilterFunction[0] = &SoftwareRenderer::FilterGrayscale;
	// if ((DrawPixelFilter & 0x4) == 0x4)
	// 	SetFilterFunction[3] = &SoftwareRenderer::FilterFadeToBlack;
}
PUBLIC STATIC int     SoftwareRenderer::GetFilter() {
	// return DrawPixelFilter;
    return 0;
}

// Utility functions
PUBLIC STATIC bool    SoftwareRenderer::IsOnScreen(int x, int y, int w, int h) {
	// return (
	// 	x + w >= Clip[0] &&
	// 	y + h >= Clip[1] &&
	// 	x     <  Clip[2] &&
	// 	y     <  Clip[3]);
    return false;
}

// Drawing Shapes
PUBLIC STATIC void    SoftwareRenderer::DrawRectangleStroke(int x, int y, int width, int height, Uint32 color) {
	int x1, x2, y1, y2;

	x1 = x;
	x2 = x + width - 1;
	if (width < 0)
		x1 += width, x2 -= width;

	y1 = y;
	y2 = y + height - 1;
	if (height < 0)
		y1 += height, y2 -= height;

	for (int b = x1; b <= x2; b++) {
		SetPixel(b, y1, color);
		SetPixel(b, y2, color);
	}
	for (int b = y1 + 1; b < y2; b++) {
		SetPixel(x1, b, color);
		SetPixel(x2, b, color);
	}
}
PUBLIC STATIC void    SoftwareRenderer::DrawRectangleSkewedH(int x, int y, int width, int height, int sk, Uint32 color) {

}

// Drawing Sprites
PUBLIC STATIC void    SoftwareRenderer::DrawSpriteNormal(ISprite* sprite, int SrcX, int SrcY, int Width, int Height, int CenterX, int CenterY, bool FlipX, bool FlipY, int RealCenterX, int RealCenterY) {
	// Uint32 PC;
	// bool AltPal = false;
	// Uint32* Pal = sprite->Palette;
	// int DX = 0, DY = 0, PX, PY, size = Width * Height;
	// // Uint32 TrPal = sprite->Palette[sprite->TransparentColorIndex];
	// Width--;
	// Height--;
	//
	// if (!AltPal && CenterY + RealCenterY >= WaterPaletteStartLine && sprite->PaletteCount == 2) {
	// 	AltPal = true;
	// 	Pal = sprite->PaletteAlt;
	// 	// TrPal = sprite->PaletteAlt[sprite->TransparentColorIndex];
	// }
	// for (int D = 0; D < size; D++) {
	// 	PX = DX;
	// 	PY = DY;
	// 	if (FlipX)
	// 		PX = Width - PX;
	// 	if (FlipY)
	// 		PY = Height - PY;
	//
	// 	PC = GetPixelIndex(sprite, SrcX + PX, SrcY + PY);
	// 	if (sprite->PaletteCount == 0) {
	// 		SetPixel(CenterX + DX + RealCenterX, CenterY + DY + RealCenterY, PC);
	// 	}
	// 	else if (PC) {
	// 		SetPixel(CenterX + DX + RealCenterX, CenterY + DY + RealCenterY, Pal[PC]);
	// 	}
	//
	// 	DX++;
	// 	if (DX > Width) {
	// 		DX = 0;
	// 		DY++;
	//
	// 		if (!AltPal && CenterY + DY + RealCenterY >= WaterPaletteStartLine && sprite->PaletteCount == 2) {
	// 			AltPal = true;
	// 			Pal = sprite->PaletteAlt;
	// 			// TrPal = sprite->PaletteAlt[sprite->TransparentColorIndex];
	// 		}
	// 	}
	// }
}
PUBLIC STATIC void    SoftwareRenderer::DrawSpriteTransformed(ISprite* sprite, int animation, int frame, int x, int y, bool flipX, bool flipY, int scaleW, int scaleH, int angle) {
	if (Graphics::SpriteRangeCheck(sprite, animation, frame)) return;

	// AnimFrame animframe = sprite->Animations[animation].Frames[frame];

	// TODO: Add shortcut here if angle == 0 && scaleW == 0x100 && scaleH = 0x100
	// DrawSpriteNormal();
	// return;

	/*

	int srcx = 0, srcy = 0, srcw = animframe.W, srch = animframe.H;
	if (srcw >= animframe.Width- srcx)
		srcw  = animframe.Width- srcx;
	if (srch >= animframe.H - srcy)
		srch  = animframe.H - srcy;

	bool AltPal = false;
	Uint32 PixelIndex, *Pal = sprite->Palette;
	int DX = 0, DY = 0, PX, PY, size;
	int CenterX = x, CenterY = y, RealCenterX = animframe.OffX, RealCenterY = animframe.OffY;
	int SrcX = animframe.X + srcx, SrcY = animframe.Y + srcy, Width = srcw - 1, Height = srch - 1;
	if (flipX)
		RealCenterX = -RealCenterX - Width - 1;
	if (flipY)
		RealCenterY = -RealCenterY - Height - 1;

	int minX = 0, maxX = 0, minY = 0, maxY = 0,
		c0x, c0y,
		mcos = Math::Cos(-angle), msin = Math::Sin(-angle),
		left = RealCenterX, top = RealCenterY;
	c0x = ((left) * mcos - (top) * msin); minX = Math::Min(minX, c0x); maxX = Math::Max(maxX, c0x);
	c0y = ((left) * msin + (top) * mcos); minY = Math::Min(minY, c0y); maxY = Math::Max(maxY, c0y);
	left += Width;
	c0x = ((left) * mcos - (top) * msin); minX = Math::Min(minX, c0x); maxX = Math::Max(maxX, c0x);
	c0y = ((left) * msin + (top) * mcos); minY = Math::Min(minY, c0y); maxY = Math::Max(maxY, c0y);
	top += Height;
	c0x = ((left) * mcos - (top) * msin); minX = Math::Min(minX, c0x); maxX = Math::Max(maxX, c0x);
	c0y = ((left) * msin + (top) * mcos); minY = Math::Min(minY, c0y); maxY = Math::Max(maxY, c0y);
	left = RealCenterX;
	c0x = ((left) * mcos - (top) * msin); minX = Math::Min(minX, c0x); maxX = Math::Max(maxX, c0x);
	c0y = ((left) * msin + (top) * mcos); minY = Math::Min(minY, c0y); maxY = Math::Max(maxY, c0y);

	minX >>= 16; maxX >>= 16;
	minY >>= 16; maxY >>= 16;

	size = (maxX - minX + 1) * (maxY - minY + 1);

	minX *= scaleW; maxX *= scaleW;
	minY *= scaleH; maxY *= scaleH;
	size *= scaleW * scaleH;

	minX >>= 8; maxX >>= 8;
	minY >>= 8; maxY >>= 8;
	size >>= 16;

	// DrawRectangle(minX + CenterX, minY + CenterY, maxX - minX + 1, maxY - minY + 1, 0xFF00FF);


	mcos = Math::Cos(angle);
	msin = Math::Sin(angle);

	if (!AltPal && CenterY + DY + minY >= WaterPaletteStartLine)
		AltPal = true, Pal = sprite->PaletteAlt;

	int NPX, NPY, X, Y, FX, FY;
	for (int D = 0; D < size; D++) {
		PX = DX + minX;
		PY = DY + minY;

		NPX = PX;
		NPY = PY;
		NPX = (NPX << 8) / scaleW;
		NPY = (NPY << 8) / scaleH;
		X = (NPX * mcos - NPY * msin) >> 16;
		Y = (NPX * msin + NPY * mcos) >> 16;

		FX = CenterX + PX;
		FY = CenterY + PY;
		if (FX <  Clip[0]) goto CONTINUE;
		if (FY <  Clip[1]) goto CONTINUE;
		if (FX >= Clip[2]) goto CONTINUE;
		if (FY >= Clip[3]) return;

		if (X >= RealCenterX && Y >= RealCenterY && X <= RealCenterX + Width && Y <= RealCenterY + Height) {
			int SX = X - RealCenterX;
			int SY = Y - RealCenterY;
			if (flipX)
				SX = Width - SX;
			if (flipY)
				SY = Height - SY;

			PixelIndex = GetPixelIndex(sprite, SrcX + SX, SrcY + SY);
			if (sprite->PaletteCount) {
				if (PixelIndex != sprite->TransparentColorIndex)
					SetPixelFunction(FX, FY, GetPixelOutputColor(Pal[PixelIndex]));
					// SetPixelFunction(FX, FY, Pal[PixelIndex]);
			}
			else if (PixelIndex & 0xFF000000U) {
				SetPixelFunction(FX, FY, GetPixelOutputColor(PixelIndex));
				// SetPixelFunction(FX, FY, PixelIndex);
			}
		}

		CONTINUE:

		DX++;
		if (DX >= maxX - minX + 1) {
			DX = 0, DY++;

			if (!AltPal && CenterY + PY >= WaterPaletteStartLine)
				AltPal = true, Pal = sprite->PaletteAlt;
		}
	}
	//*/
}

PUBLIC STATIC void    SoftwareRenderer::DrawSpriteSizedTransformed(ISprite* sprite, int animation, int frame, int x, int y, bool flipX, bool flipY, int width, int height, int angle) {
	if (Graphics::SpriteRangeCheck(sprite, animation, frame)) return;

	// ignores X/Y offsets of sprite
}
PUBLIC STATIC void    SoftwareRenderer::DrawSpriteAtlas(ISprite* sprite, int x, int y, int pivotX, int pivotY, bool flipX, bool flipY) {
	if (!sprite) return;

	// bool AltPal = false;
	// Uint32 PixelIndex, *Pal = sprite->Palette;
	//
	// int DX = 0, DY = 0, PX, PY, size = sprite->Width * sprite->Height;
	// int CenterX = x, CenterY = y, RealCenterX = -pivotX, RealCenterY = -pivotY;
	// int SrcX = 0, SrcY = 0, Width = sprite->Width - 1, Height = sprite->Height - 1;
	//
	// if (!AltPal && CenterY + RealCenterY >= WaterPaletteStartLine)
	// 	AltPal = true, Pal = sprite->PaletteAlt;
	//
	// for (int D = 0; D < size; D++) {
	// 	PX = DX;
	// 	PY = DY;
	// 	if (flipX)
	// 		PX = Width - PX;
	// 	if (flipY)
	// 		PY = Height - PY;
	//
	// 	PixelIndex = GetPixelIndex(sprite, SrcX + PX, SrcY + PY);
	// 	if (sprite->PaletteCount) {
	// 		if (PixelIndex != sprite->TransparentColorIndex)
	// 			SetPixel(CenterX + DX + RealCenterX, CenterY + DY + RealCenterY, Pal[PixelIndex]);
	// 	}
	// 	else if (PixelIndex & 0xFF000000U) {
	// 		SetPixel(CenterX + DX + RealCenterX, CenterY + DY + RealCenterY, PixelIndex);
	// 	}
	//
	// 	DX++;
	// 	if (DX > Width) {
	// 		DX = 0, DY++;
	//
	// 		if (!AltPal && CenterY + DY + RealCenterY >= WaterPaletteStartLine)
	// 			AltPal = true, Pal = sprite->PaletteAlt;
	// 	}
	// }
}

// Drawing Text
PUBLIC STATIC void    SoftwareRenderer::DrawText(int x, int y, const char* string, unsigned int pixel) {
	pixel = GetPixelOutputColor(pixel);
	// for (size_t i = 0; i < strlen(string); i++) {
	// 	for (int by = 0; by < 8; by++) {
	// 		for (int bx = 0; bx < 8; bx++) {
	// 			if (Font8x8_basic[(int)string[i]][by] & 1 << bx) {
	// 				SetPixel(x + bx + i * 8, y + by, pixel);
	// 			}
	// 		}
	// 	}
	// }
}
PUBLIC STATIC void    SoftwareRenderer::DrawTextShadow(int x, int y, const char* string, unsigned int pixel) {
	DrawText(x + 1, y + 1, string, 0x000000);
	DrawText(x, y, string, pixel);
}

PUBLIC STATIC void    SoftwareRenderer::DrawTextSprite(ISprite* sprite, int animation, char first, int x, int y, const char* string) {
	AnimFrame animframe;
	for (int i = 0; i < (int)strlen(string); i++) {
		animframe = sprite->Animations[animation].Frames[string[i] - first];
		// DrawSpriteNormal(sprite, animation, string[i] - first, x - animframe.OffsetX, y, false, false);

		x += animframe.Width;
	}
}
PUBLIC STATIC int     SoftwareRenderer::MeasureTextSprite(ISprite* sprite, int animation, char first, const char* string) {
	int x = 0;
	AnimFrame animframe;
	for (int i = 0; i < (int)strlen(string); i++) {
		animframe = sprite->Animations[animation].Frames[string[i] - first];
		x += animframe.Width;
	}
	return x;
}

// Drawing 3D
PUBLIC STATIC void    SoftwareRenderer::DrawModelOn2D(IModel* model, int x, int y, double scale, int rx, int ry, int rz, Uint32 color, bool wireframe) {
	// rx &= 0xFF;
	// IMatrix3 rotateX = IMatrix3(
	// 	1, 0, 0,
	// 	0, Math::Cos(rx), Math::Sin(rx),
	// 	0, -Math::Sin(rx), Math::Cos(rx));
    //
	// ry &= 0xFF;
	// IMatrix3 rotateY = IMatrix3(
	// 	Math::Cos(ry), 0, -Math::Sin(ry),
	// 	0, 1, 0,
	// 	Math::Sin(ry), 0, Math::Cos(ry));
    //
	// rz &= 0xFF;
	// IMatrix3 rotateZ = IMatrix3(
	// 	Math::Cos(rz), -Math::Sin(rz), 0,
	// 	Math::Sin(rz), Math::Cos(rz), 0,
	// 	0, 0, 1);
    //
	// IMatrix3 transform = rotateX.Multiply(rotateY).Multiply(rotateZ);
    //
	// int size = FrameBufferSize;
	// double* zBuffer = (double*)malloc(size * sizeof(double));
	// for (int q = 0; q < size; q++) {
	// 	zBuffer[q] = -9999999999.9f;
	// }
    //
	// int wHalf = Application::WIDTH / 2;
	// int hHalf = Application::HEIGHT / 2;
    //
	// int frame = 0;
    //
	// for (int i = 0; i < (int)model->Faces.size(); i++) {
	// 	IFace t = model->Faces[i];
    //
	// 	if (t.Quad && !wireframe) {
	// 		IVertex v1 = transform.Transform(model->Vertices[frame][t.v1]).Multiply(scale, scale, scale);
	// 		IVertex v2 = transform.Transform(model->Vertices[frame][t.v3]).Multiply(scale, scale, scale);
	// 		IVertex v3 = transform.Transform(model->Vertices[frame][t.v4]).Multiply(scale, scale, scale);
    //
	// 		// Offset model
	// 		v1.x += x;
	// 		v1.y += y;
	// 		v2.x += x;
	// 		v2.y += y;
	// 		v3.x += x;
	// 		v3.y += y;
    //
	// 		IVertex n1 = transform.Transform(model->Normals[frame][t.v1].Normalize());
	// 		IVertex n2 = transform.Transform(model->Normals[frame][t.v3].Normalize());
	// 		IVertex n3 = transform.Transform(model->Normals[frame][t.v4].Normalize());
    //
	// 		IVertex varying_intensity;
	// 		IVertex lightdir = IVertex(0.0, -1.0, 0.0);
    //
	// 		varying_intensity.x = fmax(0.0, lightdir.DotProduct(n1));
	// 		varying_intensity.y = fmax(0.0, lightdir.DotProduct(n2));
	// 		varying_intensity.z = fmax(0.0, lightdir.DotProduct(n3));
    //
	// 		double intensity = varying_intensity.Distance();
    //
	// 		int minX = (int)fmax(0.0, ceil(fmin(v1.x, fmin(v2.x, v3.x))));
	// 		int maxX = (int)fmin(wHalf * 2 - 1.0, floor(fmax(v1.x, fmax(v2.x, v3.x))));
	// 		int minY = (int)fmax(0.0, ceil(fmin(v1.y, fmin(v2.y, v3.y))));
	// 		int maxY = (int)fmin(hHalf * 2 - 1.0, floor(fmax(v1.y, fmax(v2.y, v3.y))));
    //
	// 		double triangleArea = (v1.y - v3.y) * (v2.x - v3.x) + (v2.y - v3.y) * (v3.x - v1.x);
    //
	// 		for (int y = minY; y <= maxY; y++) {
	// 			for (int x = minX; x <= maxX; x++) {
	// 				double b1 = ((y - v3.y) * (v2.x - v3.x) + (v2.y - v3.y) * (v3.x - x)) / triangleArea;
	// 				double b2 = ((y - v1.y) * (v3.x - v1.x) + (v3.y - v1.y) * (v1.x - x)) / triangleArea;
	// 				double b3 = ((y - v2.y) * (v1.x - v2.x) + (v1.y - v2.y) * (v2.x - x)) / triangleArea;
	// 				if (b1 >= 0 && b1 <= 1 && b2 >= 0 && b2 <= 1 && b3 >= 0 && b3 <= 1) {
	// 					double depth = b1 * v1.z + b2 * v2.z + b3 * v3.z;
    //
	// 					intensity = (double)varying_intensity.DotProduct(b1, b2, b3);
    //
	// 					int zIndex = y * wHalf * 2 + x;
	// 					if (zBuffer[zIndex] < depth) {
	// 						if (intensity > 0.5)
	// 							SetPixel(x, y, ColorBlend(color, 0xFFFFFF, intensity * 2.0 - 1.0));
	// 						else
	// 							SetPixel(x, y, ColorBlend(color, 0x000000, (0.5 - intensity)));
	// 						zBuffer[zIndex] = depth;
	// 					}
	// 				}
	// 			}
	// 		}
	// 	}
    //
	// 	IVertex v1 = transform.Transform(model->Vertices[frame][t.v1]).Multiply((float)scale, (float)scale, (float)scale);
	// 	IVertex v2 = transform.Transform(model->Vertices[frame][t.v2]).Multiply((float)scale, (float)scale, (float)scale);
	// 	IVertex v3 = transform.Transform(model->Vertices[frame][t.v3]).Multiply((float)scale, (float)scale, (float)scale);
    //
	// 	// Offset model
	// 	v1.x += x;
	// 	v1.y += y;
	// 	v2.x += x;
	// 	v2.y += y;
	// 	v3.x += x;
	// 	v3.y += y;
    //
	// 	IVertex n1 = transform.Transform(model->Normals[frame][t.v1].Normalize());
	// 	IVertex n2 = transform.Transform(model->Normals[frame][t.v2].Normalize());
	// 	IVertex n3 = transform.Transform(model->Normals[frame][t.v3].Normalize());
    //
	// 	IVertex varying_intensity;
	// 	IVertex lightdir = IVertex(0.0, -1.0, 0.0);
    //
	// 	varying_intensity.x = fmax(0.0f, lightdir.DotProduct(n1));
	// 	varying_intensity.y = fmax(0.0f, lightdir.DotProduct(n2));
	// 	varying_intensity.z = fmax(0.0f, lightdir.DotProduct(n3));
    //
	// 	double intensity;// = varying_intensity.Distance();
    //
	// 	int minX = (int)fmax(0.0, ceil(fmin(v1.x, fmin(v2.x, v3.x))));
	// 	int maxX = (int)fmin(wHalf * 2 - 1.0, floor(fmax(v1.x, fmax(v2.x, v3.x))));
	// 	int minY = (int)fmax(0.0, ceil(fmin(v1.y, fmin(v2.y, v3.y))));
	// 	int maxY = (int)fmin(hHalf * 2 - 1.0, floor(fmax(v1.y, fmax(v2.y, v3.y))));
    //
	// 	if (wireframe) {
	// 		intensity = fmin(1.0, fmax(0.0, varying_intensity.Distance()));
    //
	// 		if (intensity > 0.5) {
	// 			DrawLine((int)v1.x, (int)v1.y, (int)v2.x, (int)v2.y, ColorBlend(color, 0xFFFFFF, intensity * 2.0 - 1.0));
	// 			DrawLine((int)v3.x, (int)v3.y, (int)v2.x, (int)v2.y, ColorBlend(color, 0xFFFFFF, intensity * 2.0 - 1.0));
	// 		}
	// 		else {
	// 			DrawLine((int)v1.x, (int)v1.y, (int)v2.x, (int)v2.y, ColorBlend(color, 0x000000, (0.5 - intensity)));
	// 			DrawLine((int)v3.x, (int)v3.y, (int)v2.x, (int)v2.y, ColorBlend(color, 0x000000, (0.5 - intensity)));
	// 		}
	// 	}
	// 	else {
	// 		double triangleArea = (v1.y - v3.y) * (v2.x - v3.x) + (v2.y - v3.y) * (v3.x - v1.x);
    //
	// 		for (int y = minY; y <= maxY; y++) {
	// 			for (int x = minX; x <= maxX; x++) {
	// 				double b1 = ((y - v3.y) * (v2.x - v3.x) + (v2.y - v3.y) * (v3.x - x)) / triangleArea;
	// 				double b2 = ((y - v1.y) * (v3.x - v1.x) + (v3.y - v1.y) * (v1.x - x)) / triangleArea;
	// 				double b3 = ((y - v2.y) * (v1.x - v2.x) + (v1.y - v2.y) * (v2.x - x)) / triangleArea;
	// 				if (b1 >= 0 && b1 <= 1 && b2 >= 0 && b2 <= 1 && b3 >= 0 && b3 <= 1) {
	// 					double depth = b1 * v1.z + b2 * v2.z + b3 * v3.z;
	// 					// b1, b2, b3 make up "bar"; a Vector3
    //
	// 					// fragment
	// 					intensity = varying_intensity.DotProduct((float)b1, (float)b2, (float)b3);
    //
	// 					int zIndex = y * wHalf * 2 + x;
	// 					if (zBuffer[zIndex] < depth) {
	// 						if (intensity > 0.5)
	// 							SetPixel(x, y, ColorBlend(color, 0xFFFFFF, intensity * 2.0 - 1.0));
	// 						else
	// 							SetPixel(x, y, ColorBlend(color, 0x000000, (0.5 - intensity)));
	// 						zBuffer[zIndex] = depth;
	// 					}
	// 				}
	// 			}
	// 		}
	// 	}
	// }
    //
	// free(zBuffer);
}
PUBLIC STATIC void    SoftwareRenderer::DrawSpriteIn3D(ISprite* sprite, int animation, int frame, int x, int y, int z, double scale, int rx, int ry, int rz) {
	// rx &= 0xFF;
	// IMatrix3 rotateX = IMatrix3(
	// 	1, 0, 0,
	// 	0, Math::Cos(rx), Math::Sin(rx),
	// 	0, -Math::Sin(rx), Math::Cos(rx));
    //
	// ry &= 0xFF;
	// IMatrix3 rotateY = IMatrix3(
	// 	Math::Cos(ry), 0, -Math::Sin(ry),
	// 	0, 1, 0,
	// 	Math::Sin(ry), 0, Math::Cos(ry));
    //
	// rz &= 0xFF;
	// IMatrix3 rotateZ = IMatrix3(
	// 	Math::Cos(rz), -Math::Sin(rz), 0,
	// 	Math::Sin(rz), Math::Cos(rz), 0,
	// 	0, 0, 1);
    //
	// IMatrix3 transform = rotateX.Multiply(rotateY).Multiply(rotateZ);
    //
	// int size = FrameBufferSize;
	// double* zBuffer = (double*)malloc(size * sizeof(double));
	// for (int q = 0; q < size; q++) {
	// 	zBuffer[q] = -9999999999.9f;
	// }
    //
	// int wHalf = Application::WIDTH / 2;
	// int hHalf = Application::HEIGHT / 2;
    //
	// IFace t(1, 0, 2, 3);
	// vector<IVertex> Vertices;
	// vector<IVertex2> UVs;
    //
	// IMatrix2x3 varying_uv;
    //
	// Vertices.push_back(IVertex(0.0, 0.0, -0.5));
	// Vertices.push_back(IVertex(1.0, 0.0, -0.5));
	// Vertices.push_back(IVertex(0.0, 1.0, -0.5));
	// Vertices.push_back(IVertex(1.0, 1.0, -0.5));
    //
	// UVs.push_back(IVertex2(0.0, 0.0));
	// UVs.push_back(IVertex2(1.0, 0.0));
	// UVs.push_back(IVertex2(0.0, 1.0));
	// UVs.push_back(IVertex2(1.0, 1.0));
    //
	// /*
	// Sprite.Animations[CurrentAnimation].Frames[Frame >> 8].X,
	// Sprite.Animations[CurrentAnimation].Frames[Frame >> 8].Y,
	// Sprite.Animations[CurrentAnimation].Frames[Frame >> 8].W,
	// Sprite.Animations[CurrentAnimation].Frames[Frame >> 8].H, X - CamX, Y - CamY, 0, IE_NOFLIP,
	// Sprite.Animations[CurrentAnimation].Frames[Frame >> 8].OffX,
	// Sprite.Animations[CurrentAnimation].Frames[Frame >> 8].OffY
	// */
    //
	// AnimFrame currentFrame = sprite->Animations[animation].Frames[frame];
    //
	// x += currentFrame.OffX;
	// y += currentFrame.OffY;
    //
	// if (t.Quad) {
	// 	IVertex v1 = transform.Transform(Vertices[t.v1]).Multiply((float)scale, (float)scale, (float)scale);
	// 	IVertex v2 = transform.Transform(Vertices[t.v3]).Multiply((float)scale, (float)scale, (float)scale);
	// 	IVertex v3 = transform.Transform(Vertices[t.v4]).Multiply((float)scale, (float)scale, (float)scale);
    //
	// 	varying_uv.SetColumn(0, UVs[t.v1]);
	// 	varying_uv.SetColumn(1, UVs[t.v3]);
	// 	varying_uv.SetColumn(2, UVs[t.v4]);
    //
	// 	v1 = v1.Multiply((float)currentFrame.W, (float)currentFrame.H, 1.0);
	// 	v2 = v2.Multiply((float)currentFrame.W, (float)currentFrame.H, 1.0);
	// 	v3 = v3.Multiply((float)currentFrame.W, (float)currentFrame.H, 1.0);
    //
	// 	// Offset model
	// 	v1.x += x;
	// 	v1.y += y;
	// 	v2.x += x;
	// 	v2.y += y;
	// 	v3.x += x;
	// 	v3.y += y;
    //
	// 	int minX = (int)fmax(0.0, ceil(fmin(v1.x, fmin(v2.x, v3.x))));
	// 	int maxX = (int)fmin(wHalf * 2 - 1.0, floor(fmax(v1.x, fmax(v2.x, v3.x))));
	// 	int minY = (int)fmax(0.0, ceil(fmin(v1.y, fmin(v2.y, v3.y))));
	// 	int maxY = (int)fmin(hHalf * 2 - 1.0, floor(fmax(v1.y, fmax(v2.y, v3.y))));
    //
	// 	double triangleArea = (v1.y - v3.y) * (v2.x - v3.x) + (v2.y - v3.y) * (v3.x - v1.x);
    //
	// 	for (int y = minY; y <= maxY; y++) {
	// 		for (int x = minX; x <= maxX; x++) {
	// 			double b1 = ((y - v3.y) * (v2.x - v3.x) + (v2.y - v3.y) * (v3.x - x)) / triangleArea;
	// 			double b2 = ((y - v1.y) * (v3.x - v1.x) + (v3.y - v1.y) * (v1.x - x)) / triangleArea;
	// 			double b3 = ((y - v2.y) * (v1.x - v2.x) + (v1.y - v2.y) * (v2.x - x)) / triangleArea;
	// 			if (b1 >= 0 && b1 <= 1 && b2 >= 0 && b2 <= 1 && b3 >= 0 && b3 <= 1) {
	// 				double depth = b1 * v1.z + b2 * v2.z + b3 * v3.z;
    //
	// 				IVertex2 uv = varying_uv.Transform(IVertex((float)b1, (float)b2, (float)b3));
	// 				Uint32 color = GetPixelIndex(sprite,
	// 					currentFrame.X + (int)(uv.x * (currentFrame.W - 1)),
	// 					currentFrame.Y + (int)(uv.y * (currentFrame.H - 1)));
	// 				// color = sprite->Palette[color];
    //
	// 				int zIndex = y * wHalf * 2 + x;
	// 				if (zBuffer[zIndex] < depth) {
	// 					SetPixel(x, y, color);
	// 					zBuffer[zIndex] = depth;
	// 				}
	// 			}
	// 		}
	// 	}
	// }
    //
	// IVertex v1 = transform.Transform(Vertices[t.v1]).Multiply((float)scale, (float)scale, (float)scale);
	// IVertex v2 = transform.Transform(Vertices[t.v2]).Multiply((float)scale, (float)scale, (float)scale);
	// IVertex v3 = transform.Transform(Vertices[t.v3]).Multiply((float)scale, (float)scale, (float)scale);
    //
	// varying_uv.SetColumn(0, UVs[t.v1]);
	// varying_uv.SetColumn(1, UVs[t.v2]);
	// varying_uv.SetColumn(2, UVs[t.v3]);
    //
	// v1 = v1.Multiply((float)currentFrame.Width, (float)currentFrame.Height, 1.0);
	// v2 = v2.Multiply((float)currentFrame.Width, (float)currentFrame.Height, 1.0);
	// v3 = v3.Multiply((float)currentFrame.Width, (float)currentFrame.Height, 1.0);
	// // v1 = v1.Multiply(currentFrame.W - 1, currentFrame.H - 1, 1.0);
	// // v2 = v2.Multiply(currentFrame.W - 1, currentFrame.H - 1, 1.0);
	// // v3 = v3.Multiply(currentFrame.W - 1, currentFrame.H - 1, 1.0);
    //
	// // Offset model
	// v1.x += x;
	// v1.y += y;
	// v2.x += x;
	// v2.y += y;
	// v3.x += x;
	// v3.y += y;
    //
	// int minX = (int)fmax(0.0, ceil(fmin(v1.x, fmin(v2.x, v3.x))));
	// int maxX = (int)fmin(wHalf * 2 - 1.0, floor(fmax(v1.x, fmax(v2.x, v3.x))));
	// int minY = (int)fmax(0.0, ceil(fmin(v1.y, fmin(v2.y, v3.y))));
	// int maxY = (int)fmin(hHalf * 2 - 1.0, floor(fmax(v1.y, fmax(v2.y, v3.y))));
    //
	// double triangleArea = (v1.y - v3.y) * (v2.x - v3.x) + (v2.y - v3.y) * (v3.x - v1.x);
    //
	// for (int y = minY; y <= maxY; y++) {
	// 	for (int x = minX; x <= maxX; x++) {
	// 		double b1 = ((y - v3.y) * (v2.x - v3.x) + (v2.y - v3.y) * (v3.x - x)) / triangleArea;
	// 		double b2 = ((y - v1.y) * (v3.x - v1.x) + (v3.y - v1.y) * (v1.x - x)) / triangleArea;
	// 		double b3 = ((y - v2.y) * (v1.x - v2.x) + (v1.y - v2.y) * (v2.x - x)) / triangleArea;
	// 		if (b1 >= 0 && b1 <= 1 && b2 >= 0 && b2 <= 1 && b3 >= 0 && b3 <= 1) {
	// 			double depth = b1 * v1.z + b2 * v2.z + b3 * v3.z;
    //
	// 			IVertex2 uv = varying_uv.Transform(IVertex(b1, b2, b3));
	// 			Uint32 color = GetPixelIndex(sprite,
	// 				currentFrame.X + (int)(uv.x * (currentFrame.W - 1)),
	// 				currentFrame.Y + (int)(uv.y * (currentFrame.H - 1)));
	// 			// color = sprite->Palette[color];
    //
	// 			int zIndex = y * wHalf * 2 + x;
	// 			if (zBuffer[zIndex] < depth) {
	// 				SetPixel(x, y, color);
	// 				zBuffer[zIndex] = depth;
	// 			}
	// 		}
	// 	}
	// }
    //
	// free(zBuffer);
}

PUBLIC STATIC void    SoftwareRenderer::ConvertFromARGBtoNative(Uint32* argb, int count) {
    Uint8* color = (Uint8*)argb;
    if (Graphics::PreferredPixelFormat == SDL_PIXELFORMAT_ABGR8888) {
        for (int p = 0; p < count; p++) {
            *argb = 0xFF000000U | color[0] << 16 | color[1] << 8 | color[2];
            color += 4;
            argb++;
        }
    }
}

// Utility Functions
int ColR = 0xFF;
int ColG = 0xFF;
int ColB = 0xFF;
Uint32 ColRGB = 0xFFFFFF;
int MultTable[0x10000];
int MultTableInv[0x10000];
int MultSubTable[0x10000];
int Sin0x200[0x200];
int Cos0x200[0x200];

int FilterCurrent[0x8000];

int FilterColor[0x8000];
int FilterInvert[0x8000];
int FilterBlackAndWhite[0x8000];
int* FilterTable = NULL;
// Initialization and disposal functions
PUBLIC STATIC void     SoftwareRenderer::Init() {
    SoftwareRenderer::BackendFunctions.Init();
    // PixelFunction = SetPixelNormal;
	// FilterFunction[0] = FilterNone;
	// FilterFunction[1] = FilterNone;
	// FilterFunction[2] = FilterNone;
	// FilterFunction[3] = FilterNone;
    //
	// Deform = (Sint8*)Memory::TrackedCalloc("SoftwareRenderer::DeformMap", Application::HEIGHT, 1);
    //
	// Clip[0] = 0;
	// Clip[1] = 0;
	// Clip[2] = Application::WIDTH;
	// Clip[3] = Application::HEIGHT;
    // ClipSet = false;
    //
	// for (int i = 0; i < 0x100; i++)
	// 	DivideBy3Table[i] = i / 3;
    //
	// FrameBufferSize = Application::WIDTH * Application::HEIGHT;
	// FrameBuffer = (Uint32*)Memory::TrackedCalloc("SoftwareRenderer::FrameBuffer", FrameBufferSize, sizeof(Uint32));
	// FrameBufferStride = Application::WIDTH;
    //
	// ResourceStream* res;
    // if ((res = ResourceStream::New("Sprites/UI/DevFont.bin"))) {
    // 	res->ReadBytes(BitmapFont, 0x400);
    //     res->Close();
    // }
}
PUBLIC STATIC Uint32   SoftwareRenderer::GetWindowFlags() {
    return Graphics::Internal.GetWindowFlags();
}
PUBLIC STATIC void     SoftwareRenderer::SetGraphicsFunctions() {
    for (int a = 0; a < 0x100; a++) {
        for (int b = 0; b < 0x100; b++) {
            MultTable[a << 8 | b] = (a * b) >> 8;
            MultTableInv[a << 8 | b] = (a * (b ^ 0xFF)) >> 8;
            MultSubTable[a << 8 | b] = (a * -b) >> 8;
        }
    }

    for (int a = 0; a < 0x200; a++) {
        float ang = a * M_PI / 0x100;
        Sin0x200[a] = (int)(Math::Sin(ang) * 0x200);
        Cos0x200[a] = (int)(Math::Cos(ang) * 0x200);
    }
    for (int a = 0; a < 0x8000; a++) {
        int r = (a >> 10) & 0x1F;
        int g = (a >> 5) & 0x1F;
        int b = (a) & 0x1F;

        int bw = ((r + g + b) / 3) << 3;
        int hex = r << 19 | g << 11 | b << 3;
        FilterBlackAndWhite[a] = bw << 16 | bw << 8 | bw | 0xFF000000U;
        FilterInvert[a] = (hex ^ 0xFFFFFF) | 0xFF000000U;
        FilterColor[a] = hex | 0xFF000000U;

        // Game Boy-like Filter
        // bw = (((r & 0x10) + (g & 0x10) + (b & 0x10)) / 3) << 3;
        // bw <<= 1;
        // if (bw > 0xFF) bw = 0xFF;
        // r = bw * 183 / 255;
        // g = bw * 227 / 255;
        // b = bw * 42 / 255;
        // FilterBlackAndWhite[a] = b << 16 | g << 8 | r | 0xFF000000U;
    }

    FilterTable = &FilterColor[0];

    SoftwareRenderer::CurrentPalette = 0;
    for (int p = 0; p < 32; p++) {
        for (int c = 0; c < 0x100; c++) {
            SoftwareRenderer::PaletteColors[p][c]  = 0xFF000000U;
            SoftwareRenderer::PaletteColors[p][c] |= (c & 0x07) << 5; // Red?
            SoftwareRenderer::PaletteColors[p][c] |= (c & 0x18) << 11; // Green?
            SoftwareRenderer::PaletteColors[p][c] |= (c & 0xE0) << 16; // Blue?
        }
    }
    memset(SoftwareRenderer::PaletteIndexLines, 0, sizeof(SoftwareRenderer::PaletteIndexLines));

    SoftwareRenderer::BackendFunctions.Init = SoftwareRenderer::Init;
    SoftwareRenderer::BackendFunctions.GetWindowFlags = SoftwareRenderer::GetWindowFlags;
    SoftwareRenderer::BackendFunctions.Dispose = SoftwareRenderer::Dispose;

    // Texture management functions
    SoftwareRenderer::BackendFunctions.CreateTexture = SoftwareRenderer::CreateTexture;
    SoftwareRenderer::BackendFunctions.LockTexture = SoftwareRenderer::LockTexture;
    SoftwareRenderer::BackendFunctions.UpdateTexture = SoftwareRenderer::UpdateTexture;
    // SoftwareRenderer::BackendFunctions.UpdateYUVTexture = SoftwareRenderer::UpdateTextureYUV;
    SoftwareRenderer::BackendFunctions.UnlockTexture = SoftwareRenderer::UnlockTexture;
    SoftwareRenderer::BackendFunctions.DisposeTexture = SoftwareRenderer::DisposeTexture;

    // Viewport and view-related functions
    SoftwareRenderer::BackendFunctions.SetRenderTarget = SoftwareRenderer::SetRenderTarget;
    SoftwareRenderer::BackendFunctions.UpdateWindowSize = SoftwareRenderer::UpdateWindowSize;
    SoftwareRenderer::BackendFunctions.UpdateViewport = SoftwareRenderer::UpdateViewport;
    SoftwareRenderer::BackendFunctions.UpdateClipRect = SoftwareRenderer::UpdateClipRect;
    SoftwareRenderer::BackendFunctions.UpdateOrtho = SoftwareRenderer::UpdateOrtho;
    SoftwareRenderer::BackendFunctions.UpdatePerspective = SoftwareRenderer::UpdatePerspective;
    SoftwareRenderer::BackendFunctions.UpdateProjectionMatrix = SoftwareRenderer::UpdateProjectionMatrix;

    // Shader-related functions
    SoftwareRenderer::BackendFunctions.UseShader = SoftwareRenderer::UseShader;
    SoftwareRenderer::BackendFunctions.SetUniformF = SoftwareRenderer::SetUniformF;
    SoftwareRenderer::BackendFunctions.SetUniformI = SoftwareRenderer::SetUniformI;
    SoftwareRenderer::BackendFunctions.SetUniformTexture = SoftwareRenderer::SetUniformTexture;

    // These guys
    SoftwareRenderer::BackendFunctions.Clear = SoftwareRenderer::Clear;
    SoftwareRenderer::BackendFunctions.Present = SoftwareRenderer::Present;

    // Draw mode setting functions
    SoftwareRenderer::BackendFunctions.SetBlendColor = SoftwareRenderer::SetBlendColor;
    SoftwareRenderer::BackendFunctions.SetBlendMode = SoftwareRenderer::SetBlendMode;
    SoftwareRenderer::BackendFunctions.SetLineWidth = SoftwareRenderer::SetLineWidth;

    // Primitive drawing functions
    SoftwareRenderer::BackendFunctions.StrokeLine = SoftwareRenderer::StrokeLine;
    SoftwareRenderer::BackendFunctions.StrokeCircle = SoftwareRenderer::StrokeCircle;
    SoftwareRenderer::BackendFunctions.StrokeEllipse = SoftwareRenderer::StrokeEllipse;
    SoftwareRenderer::BackendFunctions.StrokeRectangle = SoftwareRenderer::StrokeRectangle;
    SoftwareRenderer::BackendFunctions.FillCircle = SoftwareRenderer::FillCircle;
    SoftwareRenderer::BackendFunctions.FillEllipse = SoftwareRenderer::FillEllipse;
    SoftwareRenderer::BackendFunctions.FillTriangle = SoftwareRenderer::FillTriangle;
    SoftwareRenderer::BackendFunctions.FillRectangle = SoftwareRenderer::FillRectangle;

    // Texture drawing functions
    SoftwareRenderer::BackendFunctions.DrawTexture = SoftwareRenderer::DrawTexture;
    SoftwareRenderer::BackendFunctions.DrawSprite = SoftwareRenderer::DrawSprite;
    SoftwareRenderer::BackendFunctions.DrawSpritePart = SoftwareRenderer::DrawSpritePart;
    SoftwareRenderer::BackendFunctions.MakeFrameBufferID = SoftwareRenderer::MakeFrameBufferID;
}
PUBLIC STATIC void     SoftwareRenderer::Dispose() {
	// Memory::Free(Deform);
	// Memory::Free(FrameBuffer);
	// SDL_FreeSurface(WindowScreen);
	// SDL_DestroyTexture(ScreenTexture);
	// SDL_DestroyWindow(Window);
    //
	// Screen->pixels = ScreenOriginalPixels;
	// SDL_FreeSurface(Screen);
    SoftwareRenderer::BackendFunctions.Dispose();
}

// Texture management functions
PUBLIC STATIC Texture* SoftwareRenderer::CreateTexture(Uint32 format, Uint32 access, Uint32 width, Uint32 height) {
    Texture* texture = Texture::New(format, access, width, height);

    return NULL;
}
PUBLIC STATIC int      SoftwareRenderer::LockTexture(Texture* texture, void** pixels, int* pitch) {
    return 0;
}
PUBLIC STATIC int      SoftwareRenderer::UpdateTexture(Texture* texture, SDL_Rect* src, void* pixels, int pitch) {
    return 0;
}
PUBLIC STATIC int      SoftwareRenderer::UpdateYUVTexture(Texture* texture, SDL_Rect* src, Uint8* pixelsY, int pitchY, Uint8* pixelsU, int pitchU, Uint8* pixelsV, int pitchV) {
    return 0;
}
PUBLIC STATIC void     SoftwareRenderer::UnlockTexture(Texture* texture) {

}
PUBLIC STATIC void     SoftwareRenderer::DisposeTexture(Texture* texture) {

}

// Viewport and view-related functions
PUBLIC STATIC void     SoftwareRenderer::SetRenderTarget(Texture* texture) {

}
PUBLIC STATIC void     SoftwareRenderer::UpdateWindowSize(int width, int height) {
    Graphics::Internal.UpdateWindowSize(width, height);
}
PUBLIC STATIC void     SoftwareRenderer::UpdateViewport() {
    Graphics::Internal.UpdateViewport();
}
PUBLIC STATIC void     SoftwareRenderer::UpdateClipRect() {
    Graphics::Internal.UpdateClipRect();
}
PUBLIC STATIC void     SoftwareRenderer::UpdateOrtho(float left, float top, float right, float bottom) {
    Graphics::Internal.UpdateOrtho(left, top, right, bottom);
}
PUBLIC STATIC void     SoftwareRenderer::UpdatePerspective(float fovy, float aspect, float nearv, float farv) {
    Graphics::Internal.UpdatePerspective(fovy, aspect, nearv, farv);
}
PUBLIC STATIC void     SoftwareRenderer::UpdateProjectionMatrix() {
    Graphics::Internal.UpdateProjectionMatrix();
}

// Shader-related functions
PUBLIC STATIC void     SoftwareRenderer::UseShader(void* shader) {
    if (!shader) {
        FilterTable = &FilterColor[0];
        return;
    }

    ObjArray* array = (ObjArray*)shader;

    if (Graphics::PreferredPixelFormat == SDL_PIXELFORMAT_ARGB8888) {
        for (int i = 0, iSz = array->Values->size(); i < 0x8000 && i < iSz; i++) {
            FilterCurrent[i] = AS_INTEGER((*array->Values)[i]) | 0xFF000000U;
        }
    }
    else {
        Uint8 px[4];
        Uint32 newI;
        for (int i = 0, iSz = array->Values->size(); i < 0x8000 && i < iSz; i++) {
            *(Uint32*)&px[0] = AS_INTEGER((*array->Values)[i]);
            newI = (i & 0x1F) << 10 | (i & 0x3E0) | (i & 0x7C00) >> 10;
            FilterCurrent[newI] = px[0] << 16 | px[1] << 8 | px[2] | 0xFF000000U;
        }
    }
    FilterTable = &FilterCurrent[0];
}
PUBLIC STATIC void     SoftwareRenderer::SetUniformF(int location, int count, float* values) {

}
PUBLIC STATIC void     SoftwareRenderer::SetUniformI(int location, int count, int* values) {

}
PUBLIC STATIC void     SoftwareRenderer::SetUniformTexture(Texture* texture, int uniform_index, int slot) {

}

// These guys
PUBLIC STATIC void     SoftwareRenderer::Clear() {
    Uint32* dstPx = (Uint32*)Graphics::CurrentRenderTarget->Pixels;
    Uint32  dstStride = Graphics::CurrentRenderTarget->Width;
    memset(dstPx, 0, dstStride * Graphics::CurrentRenderTarget->Height * 4);
}
PUBLIC STATIC void     SoftwareRenderer::Present() {

}

// Draw mode setting functions
PUBLIC STATIC void     SoftwareRenderer::SetBlendColor(float r, float g, float b, float a) {
    ColR = (int)(r * 0xFF);
    ColG = (int)(g * 0xFF);
    ColB = (int)(b * 0xFF);

    ColRGB = 0xFF000000U | ColR << 16 | ColG << 8 | ColB;
    SoftwareRenderer::ConvertFromARGBtoNative(&ColRGB, 1);

    if (a >= 1.0) {
        Alpha = 0xFF;
        // BlendFlag = 0;
        return;
    }
    Alpha = (int)(a * 0xFF);
    // BlendFlag = 1;
}
PUBLIC STATIC void     SoftwareRenderer::SetBlendMode(int srcC, int dstC, int srcA, int dstA) {
    switch (Graphics::BlendMode) {
        case BlendMode_NORMAL:
            BlendFlag = 1;
            break;
        case BlendMode_ADD:
            BlendFlag = 2;
            break;
        case BlendMode_SUBTRACT:
            BlendFlag = 3;
            break;
        case BlendMode_MATCH_EQUAL:
            BlendFlag = 4;
            break;
        case BlendMode_MATCH_NOT_EQUAL:
            BlendFlag = 5;
            break;
    }
}

PUBLIC STATIC void     SoftwareRenderer::Resize(int width, int height) {
    // Deform = (Sint8*)Memory::Realloc(Deform, height);
    // // memset(Deform + last_height, 0, height - last_height);
    //
    // if (!ClipSet) {
    //     Clip[0] = 0;
    // 	Clip[1] = 0;
    // 	Clip[2] = width;
    // 	Clip[3] = height;
    // }
    //
	// FrameBuffer = (Uint32*)Memory::Realloc(FrameBuffer, width * height * sizeof(Uint32));
    // if (width * height - FrameBufferSize > 0)
    //     memset(FrameBuffer + FrameBufferSize, 0, width * height - FrameBufferSize);
    // FrameBufferSize = width * height;
	// FrameBufferStride = width;
}

PUBLIC STATIC void     SoftwareRenderer::SetClip(float x, float y, float width, float height) {
    // ClipSet = true;
    //
    // Clip[0] = (int)x;
	// Clip[1] = (int)y;
	// Clip[2] = (int)x + (int)w;
	// Clip[3] = (int)y + (int)h;
    //
	// if (Clip[0] < 0)
	// 	Clip[0] = 0;
	// if (Clip[1] < 0)
	// 	Clip[1] = 0;
	// if (Clip[2] > Application::WIDTH)
	// 	Clip[2] = Application::WIDTH;
	// if (Clip[3] > Application::HEIGHT)
	// 	Clip[3] = Application::HEIGHT;
}
PUBLIC STATIC void     SoftwareRenderer::ClearClip() {
    // ClipSet = false;
    //
    // Clip[0] = 0;
	// Clip[1] = 0;
	// Clip[2] = Application::WIDTH;
	// Clip[3] = Application::HEIGHT;
}

PUBLIC STATIC void     SoftwareRenderer::Save() {

}
PUBLIC STATIC void     SoftwareRenderer::Translate(float x, float y, float z) {

}
PUBLIC STATIC void     SoftwareRenderer::Rotate(float x, float y, float z) {

}
PUBLIC STATIC void     SoftwareRenderer::Scale(float x, float y, float z) {

}
PUBLIC STATIC void     SoftwareRenderer::Restore() {

}

#define SRC_CHECK false

// Filter versions
inline void PixelSetOpaque(Uint32* src, Uint32* dst, int opacity, int* multTableAt, int* multSubTableAt) {
    // if ((*src) & 0xFF000000U) {
        register Uint32 col = *src;
        *dst = FilterTable[(col & 0xF80000) >> 9 | (col & 0xF800) >> 6 | (col & 0xF8) >> 3];
        // *dst = col;
    // }
}
inline void PixelSetTransparent(Uint32* src, Uint32* dst, int opacity, int* multTableAt, int* multSubTableAt) {
    if (SRC_CHECK) {
        register Uint32 col = *src;
        col = FilterTable[(col & 0xF80000) >> 9 | (col & 0xF800) >> 6 | (col & 0xF8) >> 3];
        *dst = ColorBlend(*dst, col, opacity) | 0xFF000000U;
    }
}
inline void PixelSetAdditive(Uint32* src, Uint32* dst, int opacity, int* multTableAt, int* multSubTableAt) {
    if (SRC_CHECK) {
        register Uint32 col = *src;
        col = FilterTable[(col & 0xF80000) >> 9 | (col & 0xF800) >> 6 | (col & 0xF8) >> 3];

        Uint32 R = (multTableAt[(col >> 16) & 0xFF] << 16) + (*dst & 0xFF0000);
        Uint32 G = (multTableAt[(col >> 8) & 0xFF] << 8) + (*dst & 0x00FF00);
        Uint32 B = (multTableAt[(col) & 0xFF]) + (*dst & 0x0000FF);
        if (R > 0xFF0000) R = 0xFF0000;
        if (G > 0x00FF00) G = 0x00FF00;
        if (B > 0x0000FF) B = 0x0000FF;
        *dst = R | G | B | 0xFF000000U;
    }
}
inline void PixelSetSubtract(Uint32* src, Uint32* dst, int opacity, int* multTableAt, int* multSubTableAt) {
    if (SRC_CHECK) {
        register Uint32 col = *src;
        col = FilterTable[(col & 0xF80000) >> 9 | (col & 0xF800) >> 6 | (col & 0xF8) >> 3];

        Sint32 R = (multSubTableAt[(col >> 16) & 0xFF] << 16) + (*dst & 0xFF0000);
        Sint32 G = (multSubTableAt[(col >> 8) & 0xFF] << 8) + (*dst & 0x00FF00);
        Sint32 B = (multSubTableAt[(col) & 0xFF]) + (*dst & 0x0000FF);
        if (R < 0) R = 0;
        if (G < 0) G = 0;
        if (B < 0) B = 0;
        *dst = R | G | B | 0xFF000000U;
    }
}
inline void PixelSetMatchEqual(Uint32* src, Uint32* dst, int opacity, int* multTableAt, int* multSubTableAt) {
    if (*dst == SoftwareRenderer::CompareColor) {
        register Uint32 col = *src;
        *dst = FilterTable[(col & 0xF80000) >> 9 | (col & 0xF800) >> 6 | (col & 0xF8) >> 3];
    }
}
inline void PixelSetMatchNotEqual(Uint32* src, Uint32* dst, int opacity, int* multTableAt, int* multSubTableAt) {
    if (*dst != SoftwareRenderer::CompareColor) {
        register Uint32 col = *src;
        *dst = FilterTable[(col & 0xF80000) >> 9 | (col & 0xF800) >> 6 | (col & 0xF8) >> 3];
    }
}

// Filterless versions
inline void PixelNoFiltSetOpaque(Uint32* src, Uint32* dst, int opacity, int* multTableAt, int* multSubTableAt) {
    *dst = *src;
}
inline void PixelNoFiltSetTransparent(Uint32* src, Uint32* dst, int opacity, int* multTableAt, int* multSubTableAt) {
    *dst = ColorBlend(*dst, *src, opacity) | 0xFF000000U;
}
inline void PixelNoFiltSetAdditive(Uint32* src, Uint32* dst, int opacity, int* multTableAt, int* multSubTableAt) {
    Uint32 R = (multTableAt[(*src >> 16) & 0xFF] << 16) + (*dst & 0xFF0000);
    Uint32 G = (multTableAt[(*src >> 8) & 0xFF] << 8) + (*dst & 0x00FF00);
    Uint32 B = (multTableAt[(*src) & 0xFF]) + (*dst & 0x0000FF);
    if (R > 0xFF0000) R = 0xFF0000;
    if (G > 0x00FF00) G = 0x00FF00;
    if (B > 0x0000FF) B = 0x0000FF;
    *dst = R | G | B | 0xFF000000U;
}
inline void PixelNoFiltSetSubtract(Uint32* src, Uint32* dst, int opacity, int* multTableAt, int* multSubTableAt) {
    Sint32 R = (multSubTableAt[(*src >> 16) & 0xFF] << 16) + (*dst & 0xFF0000);
    Sint32 G = (multSubTableAt[(*src >> 8) & 0xFF] << 8) + (*dst & 0x00FF00);
    Sint32 B = (multSubTableAt[(*src) & 0xFF]) + (*dst & 0x0000FF);
    if (R < 0) R = 0;
    if (G < 0) G = 0;
    if (B < 0) B = 0;
    *dst = R | G | B | 0xFF000000U;
}
inline void PixelNoFiltSetMatchEqual(Uint32* src, Uint32* dst, int opacity, int* multTableAt, int* multSubTableAt) {
    if (*dst == SoftwareRenderer::CompareColor) {
        *dst = *src;
    }
}
inline void PixelNoFiltSetMatchNotEqual(Uint32* src, Uint32* dst, int opacity, int* multTableAt, int* multSubTableAt) {
    if (*dst != SoftwareRenderer::CompareColor) {
        *dst = *src;
    }
}

struct Contour {
    int MinX;
    int MaxX;
};
Contour ContourField[480];
void DrawPolygonBlendScanLine(int color1, int color2, int x1, int y1, int x2, int y2) {
    int sx, sy, dx1, dy1, dx2, dy2, x, y, m, n, k, cnt;

    int minY = y1;
    int maxY = y1;

    if (y2 < minY)
        minY = y2;
    if (y2 > maxY)
        maxY = y2;

    if (Graphics::CurrentClip.Enabled) {
        if (minY < Graphics::CurrentClip.Y)
            minY = Graphics::CurrentClip.Y;
        if (maxY > Graphics::CurrentClip.Y + Graphics::CurrentClip.Height)
            maxY = Graphics::CurrentClip.Y + Graphics::CurrentClip.Height;
    }
    else {
        if (minY < 0)
            minY = 0;
        if (maxY > (int)Graphics::CurrentRenderTarget->Height)
            maxY = (int)Graphics::CurrentRenderTarget->Height;
    }

    int minX;
    int maxX;
    if (Graphics::CurrentClip.Enabled) {
        minX = Graphics::CurrentClip.X;
        maxX = Graphics::CurrentClip.X + Graphics::CurrentClip.Width;
    }
    else {
        minX = 0;
        maxX = (int)Graphics::CurrentRenderTarget->Width;
    }

	sx = x2 - x1;
	sy = y2 - y1;

	if (sx > 0) dx1 = 1;
	else if (sx < 0) dx1 = -1;
	else dx1 = 0;

	if (sy > 0) dy1 = 1;
	else if (sy < 0) dy1 = -1;
	else dy1 = 0;

	m = abs(sx);
	n = abs(sy);
	dx2 = dx1;
	dy2 = 0;

	if (m < n) {
		m = abs(sy);
		n = abs(sx);
		dx2 = 0;
		dy2 = dy1;
	}

	x = x1;
	y = y1;
	cnt = m;
	k = n / 2;

	while (cnt--) {
        if (y >= minY && y <= maxY) {
            if (x < minX) {
                ContourField[y].MinX = minX;
            }
            else if (x >= maxX) {
                ContourField[y].MaxX = maxX;
            }
            else {
                if (x < ContourField[y].MinX)
                    ContourField[y].MinX = x;
    			if (x > ContourField[y].MaxX)
                    ContourField[y].MaxX = x;
            }
		}

		k += n;
		if (k < m) {
			x += dx2;
			y += dy2;
		}
		else {
			k -= m;
			x += dx1;
			y += dy1;
		}
	}
}
void DrawPolygonBlend(int* vectors, int* colors, int count, int opacity, int blendFlag) {
    Uint32* dstPx = (Uint32*)Graphics::CurrentRenderTarget->Pixels;
    Uint32  dstStride = Graphics::CurrentRenderTarget->Width;

    int minVal = INT_MAX;
    int maxVal = INT_MIN;

    if (count == 0)
        return;

    int* tempVertex = &vectors[0];
    int  tempCount = count;
    while (tempCount--) {
        register int vy = tempVertex[1];
        if (vy < minVal)
            minVal = vy;
        if (vy > maxVal)
            maxVal = vy;

        tempVertex += 2;
    }

    int dst_y1 = minVal;
    int dst_y2 = maxVal;

    if (Graphics::CurrentClip.Enabled) {
        if (dst_y2 > Graphics::CurrentClip.Y + Graphics::CurrentClip.Height)
            dst_y2 = Graphics::CurrentClip.Y + Graphics::CurrentClip.Height;
        if (dst_y1 < Graphics::CurrentClip.Y)
            dst_y1 = Graphics::CurrentClip.Y;
    }
    else {
        if (dst_y2 > (int)Graphics::CurrentRenderTarget->Height)
            dst_y2 = (int)Graphics::CurrentRenderTarget->Height;
        if (dst_y1 < 0)
            dst_y1 = 0;
    }

    if (dst_y1 >= dst_y2)
        return;

    int scanLineCount = dst_y2 - dst_y1;
    Contour* contourPtr = &ContourField[dst_y1];
    while (scanLineCount--) {
        contourPtr->MinX = 0x7FFFFFFF;
        contourPtr->MaxX = -1;
        contourPtr++;
    }

    int* lastVector = vectors;
    int* lastColor = colors;
    if (count > 1) {
        int countRem = count - 1;
        while (countRem--) {
            DrawPolygonBlendScanLine(lastColor[0], lastColor[1], lastVector[0], lastVector[1], lastVector[2], lastVector[3]);
            lastVector += 2;
            lastColor++;
        }
    }
    DrawPolygonBlendScanLine(lastColor[0], colors[0], lastVector[0], lastVector[1], vectors[0], vectors[1]);

    // int blendFlag = BlendFlag;
    // int opacity = Alpha;
    // if (Alpha == 0 && blendFlag != 0)
    //     return;
    // if (Alpha != 0 && blendFlag == 0)
    //     blendFlag = 1;

    register Uint32 col = ColRGB;
    // col = FilterTable[(col & 0xF80000) >> 9 | (col & 0xF800) >> 6 | (col & 0xF8) >> 3];

    #define DRAW_POLYGONBLEND(pixelFunction) for (int dst_y = dst_y1; dst_y < dst_y2; dst_y++) { \
        Contour contour = ContourField[dst_y]; \
        if (contour.MaxX < contour.MinX) { \
            dst_strideY += dstStride; \
            continue; \
        } \
        for (int dst_x = contour.MinX; dst_x < contour.MaxX; dst_x++) { \
            pixelFunction((Uint32*)&col, &dstPx[dst_x + dst_strideY], opacity, multTableAt, multSubTableAt); \
        } \
        dst_strideY += dstStride; \
    }

    int* multTableAt = &MultTable[opacity << 8];
    int* multSubTableAt = &MultSubTable[opacity << 8];
    int dst_strideY = dst_y1 * dstStride;
    switch (blendFlag) {
        case 0:
            for (int dst_y = dst_y1; dst_y < dst_y2; dst_y++) {
                Contour contour = ContourField[dst_y];
                if (contour.MaxX < contour.MinX) {
                    dst_strideY += dstStride;
                    continue;
                }

                Memory::Memset4(&dstPx[contour.MinX + dst_strideY], col, contour.MaxX - contour.MinX);
                dst_strideY += dstStride;
            }
            break;
        case 1:
            DRAW_POLYGONBLEND(PixelNoFiltSetTransparent);
            break;
        case 2:
            DRAW_POLYGONBLEND(PixelNoFiltSetAdditive);
            break;
        case 3:
            DRAW_POLYGONBLEND(PixelNoFiltSetSubtract);
            break;
        case 4:
            DRAW_POLYGONBLEND(PixelNoFiltSetMatchEqual);
            break;
        case 5:
            DRAW_POLYGONBLEND(PixelNoFiltSetMatchNotEqual);
            break;
    }

    #undef DRAW_POLYGONBLEND
}

PUBLIC STATIC void     SoftwareRenderer::SetLineWidth(float n) {

}
PUBLIC STATIC void     SoftwareRenderer::StrokeLine(float x1, float y1, float x2, float y2) {
    int x = 0, y = 0;
    Uint32* dstPx = (Uint32*)Graphics::CurrentRenderTarget->Pixels;
    Uint32  dstStride = Graphics::CurrentRenderTarget->Width;

    View* currentView = &Scene::Views[Scene::ViewCurrent];
    int cx = (int)std::floor(currentView->X);
    int cy = (int)std::floor(currentView->Y);

    Matrix4x4* out = Graphics::ModelViewMatrix.top();
    x += out->Values[12];
    y += out->Values[13];
    x -= cx;
    y -= cy;

    int dst_x1 = x + x1;
    int dst_y1 = y + y1;
    int dst_x2 = x + x2;
    int dst_y2 = y + y2;

    int minX, maxX, minY, maxY;
    if (Graphics::CurrentClip.Enabled) {
        minX = Graphics::CurrentClip.X;
        minY = Graphics::CurrentClip.Y;
        maxX = Graphics::CurrentClip.X + Graphics::CurrentClip.Width;
        maxY = Graphics::CurrentClip.Y + Graphics::CurrentClip.Height;
    }
    else {
        minX = 0;
        minY = 0;
        maxX = (int)Graphics::CurrentRenderTarget->Width;
        maxY = (int)Graphics::CurrentRenderTarget->Height;
    }

    int dx = Math::Abs(dst_x2 - dst_x1), sx = dst_x1 < dst_x2 ? 1 : -1;
    int dy = Math::Abs(dst_y2 - dst_y1), sy = dst_y1 < dst_y2 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;

    int blendFlag = BlendFlag;
    int opacity = Alpha;
    if (Alpha == 0 && blendFlag != 0)
        return;
    if (Alpha != 0 && blendFlag == 0)
        blendFlag = 1;

    register Uint32 col = ColRGB;
    // col = FilterTable[(col & 0xF80000) >> 9 | (col & 0xF800) >> 6 | (col & 0xF8) >> 3];

    #define DRAW_LINE(pixelFunction) while (true) { \
        if (dst_x1 >= minX && dst_y1 >= minY && dst_x1 < maxX && dst_y1 < maxY) \
            pixelFunction((Uint32*)&col, &dstPx[dst_x1 + dst_y1 * dstStride], opacity, multTableAt, multSubTableAt); \
        if (dst_x1 == dst_x2 && dst_y1 == dst_y2) break; \
        e2 = err; \
        if (e2 > -dx) { err -= dy; dst_x1 += sx; } \
        if (e2 <  dy) { err += dx; dst_y1 += sy; } \
    }

    int* multTableAt = &MultTable[opacity << 8];
    int* multSubTableAt = &MultSubTable[opacity << 8];
    // int dst_strideY = dst_y1 * dstStride;
    switch (blendFlag) {
        case 0:
            DRAW_LINE(PixelNoFiltSetOpaque);
            break;
        case 1:
            DRAW_LINE(PixelNoFiltSetTransparent);
            break;
        case 2:
            DRAW_LINE(PixelNoFiltSetAdditive);
            break;
        case 3:
            DRAW_LINE(PixelNoFiltSetSubtract);
            break;
    }

    #undef DRAW_LINE
}
PUBLIC STATIC void     SoftwareRenderer::StrokeCircle(float x, float y, float rad) {

}
PUBLIC STATIC void     SoftwareRenderer::StrokeEllipse(float x, float y, float w, float h) {

}
PUBLIC STATIC void     SoftwareRenderer::StrokeRectangle(float x, float y, float w, float h) {

}

PUBLIC STATIC void     SoftwareRenderer::FillCircle(float x, float y, float rad) {
    // just checks to see if the pixel is within a radius range, uses a bounding box constructed by the diameter

    Uint32* dstPx = (Uint32*)Graphics::CurrentRenderTarget->Pixels;
    Uint32  dstStride = Graphics::CurrentRenderTarget->Width;

    View* currentView = &Scene::Views[Scene::ViewCurrent];
    int cx = (int)std::floor(currentView->X);
    int cy = (int)std::floor(currentView->Y);

    Matrix4x4* out = Graphics::ModelViewMatrix.top();
    x += out->Values[12];
    y += out->Values[13];
    x -= cx;
    y -= cy;

    int dst_x1 = x - rad;
    int dst_y1 = y - rad;
    int dst_x2 = x + rad;
    int dst_y2 = y + rad;

    if (Graphics::CurrentClip.Enabled) {
        if (dst_x2 > Graphics::CurrentClip.X + Graphics::CurrentClip.Width)
            dst_x2 = Graphics::CurrentClip.X + Graphics::CurrentClip.Width;
        if (dst_y2 > Graphics::CurrentClip.Y + Graphics::CurrentClip.Height)
            dst_y2 = Graphics::CurrentClip.Y + Graphics::CurrentClip.Height;

        if (dst_x1 < Graphics::CurrentClip.X)
            dst_x1 = Graphics::CurrentClip.X;
        if (dst_y1 < Graphics::CurrentClip.Y)
            dst_y1 = Graphics::CurrentClip.Y;
    }
    else {
        if (dst_x2 > (int)Graphics::CurrentRenderTarget->Width)
            dst_x2 = (int)Graphics::CurrentRenderTarget->Width;
        if (dst_y2 > (int)Graphics::CurrentRenderTarget->Height)
            dst_y2 = (int)Graphics::CurrentRenderTarget->Height;

        if (dst_x1 < 0)
            dst_x1 = 0;
        if (dst_y1 < 0)
            dst_y1 = 0;
    }

    if (dst_x1 >= dst_x2)
        return;
    if (dst_y1 >= dst_y2)
        return;

    int blendFlag = BlendFlag;
    int opacity = Alpha;
    if (Alpha == 0 && blendFlag != 0)
        return;
    if (Alpha != 0 && blendFlag == 0)
        blendFlag = 1;

    int outRad = rad * rad;
    int innRad = 0;

    int scanLineCount = dst_y2 - dst_y1 + 1;
    Contour* contourPtr = &ContourField[dst_y1];
    while (scanLineCount--) {
        contourPtr->MinX = 0x7FFFFFFF;
        contourPtr->MaxX = -1;
        contourPtr++;
    }

    #define SEEK_MIN(our_x, our_y) if (our_y >= dst_y1 && our_y <= dst_y2 && our_x < (cont = &ContourField[our_y])->MinX) \
        cont->MinX = our_x;
    #define SEEK_MAX(our_x, our_y) if (our_y >= dst_y1 && our_y <= dst_y2 && our_x > (cont = &ContourField[our_y])->MaxX) \
        cont->MaxX = our_x;

    Contour* cont;
    int ccx = x, ccy = y;
    int bx = 0, by = rad;
    int bd = 3 - 2 * rad;
    while (bx <= by) {
        if (bd <= 0) {
            bd += 4 * bx + 6;
        }
        else {
            bd += 4 * (bx - by) + 10;
            by--;
        }
        bx++;
        SEEK_MAX(ccx + bx, ccy - by);
        SEEK_MIN(ccx - bx, ccy - by);
        SEEK_MAX(ccx + by, ccy - bx);
        SEEK_MIN(ccx - by, ccy - bx);
        ccy--;
        SEEK_MAX(ccx + bx, ccy + by);
        SEEK_MIN(ccx - bx, ccy + by);
        SEEK_MAX(ccx + by, ccy + bx);
        SEEK_MIN(ccx - by, ccy + bx);
        ccy++;
    }

    register Uint32 col = ColRGB;
    // col = FilterTable[(col & 0xF80000) >> 9 | (col & 0xF800) >> 6 | (col & 0xF8) >> 3];

    #define DRAW_CIRCLE(pixelFunction) for (int dst_y = dst_y1; dst_y < dst_y2; dst_y++) { \
        Contour contour = ContourField[dst_y]; \
        if (contour.MaxX < contour.MinX) { \
            dst_strideY += dstStride; \
            continue; \
        } \
        for (int dst_x = contour.MinX >= dst_x1 ? contour.MinX : dst_x1; dst_x < contour.MaxX && dst_x < dst_x2; dst_x++) { \
            pixelFunction((Uint32*)&col, &dstPx[dst_x + dst_strideY], opacity, multTableAt, multSubTableAt); \
        } \
        dst_strideY += dstStride; \
    }

    int* multTableAt = &MultTable[opacity << 8];
    int* multSubTableAt = &MultSubTable[opacity << 8];
    int dst_strideY = dst_y1 * dstStride;
    switch (blendFlag) {
        case 0:
            for (int dst_y = dst_y1; dst_y < dst_y2; dst_y++) {
                Contour contour = ContourField[dst_y];
                if (contour.MaxX < contour.MinX) {
                    dst_strideY += dstStride;
                    continue;
                }
                int dst_min_x = contour.MinX;
                if (dst_min_x > dst_x1)
                    dst_min_x = dst_x1;
                int dst_max_x = contour.MaxX;
                if (dst_max_x > dst_x2)
                    dst_max_x = dst_x2;

                Memory::Memset4(&dstPx[dst_min_x], col, dst_max_x - dst_min_x);
                dst_strideY += dstStride;
            }

            break;
        case 1:
            DRAW_CIRCLE(PixelNoFiltSetTransparent);
            break;
        case 2:
            DRAW_CIRCLE(PixelNoFiltSetAdditive);
            break;
        case 3:
            DRAW_CIRCLE(PixelNoFiltSetSubtract);
            break;
        case 4:
            DRAW_CIRCLE(PixelNoFiltSetMatchEqual);
            break;
        case 5:
            DRAW_CIRCLE(PixelNoFiltSetMatchNotEqual);
            break;
    }

    #undef DRAW_CIRCLE
}
PUBLIC STATIC void     SoftwareRenderer::FillEllipse(float x, float y, float w, float h) {

}
PUBLIC STATIC void     SoftwareRenderer::FillRectangle(float x, float y, float w, float h) {
    Uint32* dstPx = (Uint32*)Graphics::CurrentRenderTarget->Pixels;
    Uint32  dstStride = Graphics::CurrentRenderTarget->Width;

    View* currentView = &Scene::Views[Scene::ViewCurrent];
    int cx = (int)std::floor(currentView->X);
    int cy = (int)std::floor(currentView->Y);

    Matrix4x4* out = Graphics::ModelViewMatrix.top();
    x += out->Values[12];
    y += out->Values[13];
    x -= cx;
    y -= cy;

    int dst_x1 = x;
    int dst_y1 = y;
    int dst_x2 = x + w;
    int dst_y2 = y + h;

    if (Graphics::CurrentClip.Enabled) {
        if (dst_x2 > Graphics::CurrentClip.X + Graphics::CurrentClip.Width)
            dst_x2 = Graphics::CurrentClip.X + Graphics::CurrentClip.Width;
        if (dst_y2 > Graphics::CurrentClip.Y + Graphics::CurrentClip.Height)
            dst_y2 = Graphics::CurrentClip.Y + Graphics::CurrentClip.Height;

        if (dst_x1 < Graphics::CurrentClip.X)
            dst_x1 = Graphics::CurrentClip.X;
        if (dst_y1 < Graphics::CurrentClip.Y)
            dst_y1 = Graphics::CurrentClip.Y;
    }
    else {
        if (dst_x2 > (int)Graphics::CurrentRenderTarget->Width)
            dst_x2 = (int)Graphics::CurrentRenderTarget->Width;
        if (dst_y2 > (int)Graphics::CurrentRenderTarget->Height)
            dst_y2 = (int)Graphics::CurrentRenderTarget->Height;

        if (dst_x1 < 0)
            dst_x1 = 0;
        if (dst_y1 < 0)
            dst_y1 = 0;
    }

    if (dst_x1 >= dst_x2)
        return;
    if (dst_y1 >= dst_y2)
        return;

    int blendFlag = BlendFlag;
    int opacity = Alpha;
    if (Alpha == 0 && blendFlag != 0)
        return;
    if (Alpha != 0 && blendFlag == 0)
        blendFlag = 1;

    register Uint32 col = ColRGB;
    // col = FilterTable[(col & 0xF80000) >> 9 | (col & 0xF800) >> 6 | (col & 0xF8) >> 3];

    #define DRAW_RECT(pixelFunction) for (int dst_y = dst_y1; dst_y < dst_y2; dst_y++) { \
        for (int dst_x = dst_x1; dst_x < dst_x2; dst_x++) { \
            pixelFunction((Uint32*)&col, &dstPx[dst_x + dst_strideY], opacity, multTableAt, multSubTableAt); \
        } \
        dst_strideY += dstStride; \
    }

    int* multTableAt = &MultTable[opacity << 8];
    int* multSubTableAt = &MultSubTable[opacity << 8];
    int dst_strideY = dst_y1 * dstStride;
    switch (blendFlag) {
        case 0:
            for (int dst_y = dst_y1; dst_y < dst_y2; dst_y++) {
                Memory::Memset4(&dstPx[dst_x1 + dst_strideY], col, dst_x2 - dst_x1);
                dst_strideY += dstStride;
            }
            break;
        case 1:
            DRAW_RECT(PixelNoFiltSetTransparent);
            break;
        case 2:
            DRAW_RECT(PixelNoFiltSetAdditive);
            break;
        case 3:
            DRAW_RECT(PixelNoFiltSetSubtract);
            break;
        case 4:
            DRAW_RECT(PixelNoFiltSetMatchEqual);
            break;
        case 5:
            DRAW_RECT(PixelNoFiltSetMatchNotEqual);
            break;
    }

    #undef DRAW_RECT
}
PUBLIC STATIC void     SoftwareRenderer::FillTriangle(float x1, float y1, float x2, float y2, float x3, float y3) {
    int x = 0, y = 0;
    View* currentView = &Scene::Views[Scene::ViewCurrent];
    int cx = (int)std::floor(currentView->X);
    int cy = (int)std::floor(currentView->Y);

    Matrix4x4* out = Graphics::ModelViewMatrix.top();
    x += out->Values[12];
    y += out->Values[13];
    x -= cx;
    y -= cy;

    int vectors[6] = {
        (int)x1 + x, (int)y1 + y,
        (int)x2 + x, (int)y2 + y,
        (int)x3 + x, (int)y3 + y,
    };
    int colors[3] = {
        0,
        0,
        0,
    };
    DrawPolygonBlend(vectors, colors, 3, Alpha, BlendFlag);
}

void DrawSpriteImage(Texture* texture, int x, int y, int w, int h, int sx, int sy, int flipFlag, int blendFlag, int opacity) {
    register Uint32* srcPx = (Uint32*)texture->Pixels;
    register Uint32  srcStride = texture->Width;
    register Uint32* srcPxLine;

    register Uint32* dstPx = (Uint32*)Graphics::CurrentRenderTarget->Pixels;
    register Uint32  dstStride = Graphics::CurrentRenderTarget->Width;
    register Uint32* dstPxLine;

    register int src_x1 = sx;
    register int src_y1 = sy;
    register int src_x2 = sx + w - 1;
    register int src_y2 = sy + h - 1;

    register int dst_x1 = x;
    register int dst_y1 = y;
    register int dst_x2 = x + w;
    register int dst_y2 = y + h;

    if (Alpha != 0 && blendFlag == 0)
        blendFlag = 1;
    if (!Graphics::TextureBlend)
        blendFlag = 0;

    if (Graphics::CurrentClip.Enabled) {
        register int
            clip_x1 = Graphics::CurrentClip.X,
            clip_y1 = Graphics::CurrentClip.Y,
            clip_x2 = Graphics::CurrentClip.X + Graphics::CurrentClip.Width,
            clip_y2 = Graphics::CurrentClip.Y + Graphics::CurrentClip.Height;

        if (dst_x2 > clip_x2)
            dst_x2 = clip_x2;
        if (dst_y2 > clip_y2)
            dst_y2 = clip_y2;

        if (dst_x1 < clip_x1) {
            src_x1 += clip_x1 - dst_x1;
            src_x2 -= clip_x1 - dst_x1;
            dst_x1 = clip_x1;
        }
        if (dst_y1 < clip_y1) {
            src_y1 += clip_y1 - dst_y1;
            src_y2 -= clip_y1 - dst_y1;
            dst_y1 = clip_y1;
        }
    }
    else {
        register int
            clip_x2 = (int)Graphics::CurrentRenderTarget->Width,
            clip_y2 = (int)Graphics::CurrentRenderTarget->Height;
        if (dst_x2 > clip_x2)
            dst_x2 = clip_x2;
        if (dst_y2 > clip_y2)
            dst_y2 = clip_y2;

        if (dst_x1 < 0) {
            src_x1 += -dst_x1;
            src_x2 -= -dst_x1;
            dst_x1 = 0;
        }
        if (dst_y1 < 0) {
            src_y1 += -dst_y1;
            src_y2 -= -dst_y1;
            dst_y1 = 0;
        }
    }

    if (dst_x1 >= dst_x2)
        return;
    if (dst_y1 >= dst_y2)
        return;

    #define DRAW_PLACEPIXEL(pixelFunction) \
        if ((color = srcPxLine[src_x]) & 0xFF000000U) \
            pixelFunction(&color, &dstPxLine[dst_x], opacity, multTableAt, multSubTableAt);
    #define DRAW_PLACEPIXEL_PAL(pixelFunction) \
        if ((color = srcPxLine[src_x])) \
            pixelFunction(&index[color], &dstPxLine[dst_x], opacity, multTableAt, multSubTableAt);

    #define DRAW_NOFLIP(pixelFunction, placePixelMacro) for (int dst_y = dst_y1; dst_y < dst_y2; dst_y++) { \
        srcPxLine = srcPx + src_strideY; \
        dstPxLine = dstPx + dst_strideY; \
        index = &SoftwareRenderer::PaletteColors[SoftwareRenderer::PaletteIndexLines[dst_y]][0]; \
        for (int dst_x = dst_x1, src_x = src_x1; dst_x < dst_x2; dst_x++, src_x++) { \
            placePixelMacro(pixelFunction) \
        } \
        dst_strideY += dstStride; src_strideY += srcStride; \
    }
    #define DRAW_FLIPX(pixelFunction, placePixelMacro) for (int dst_y = dst_y1; dst_y < dst_y2; dst_y++) { \
        srcPxLine = srcPx + src_strideY; \
        dstPxLine = dstPx + dst_strideY; \
        index = &SoftwareRenderer::PaletteColors[SoftwareRenderer::PaletteIndexLines[dst_y]][0]; \
        for (int dst_x = dst_x1, src_x = src_x2; dst_x < dst_x2; dst_x++, src_x--) { \
            placePixelMacro(pixelFunction) \
        } \
        dst_strideY += dstStride; src_strideY += srcStride; \
    }
    #define DRAW_FLIPY(pixelFunction, placePixelMacro) for (int dst_y = dst_y1; dst_y < dst_y2; dst_y++) { \
        srcPxLine = srcPx + src_strideY; \
        dstPxLine = dstPx + dst_strideY; \
        index = &SoftwareRenderer::PaletteColors[SoftwareRenderer::PaletteIndexLines[dst_y]][0]; \
        for (int dst_x = dst_x1, src_x = src_x1; dst_x < dst_x2; dst_x++, src_x++) { \
            placePixelMacro(pixelFunction) \
        } \
        dst_strideY += dstStride; src_strideY -= srcStride; \
    }
    #define DRAW_FLIPXY(pixelFunction, placePixelMacro) for (int dst_y = dst_y1; dst_y < dst_y2; dst_y++) { \
        srcPxLine = srcPx + src_strideY; \
        dstPxLine = dstPx + dst_strideY; \
        index = &SoftwareRenderer::PaletteColors[SoftwareRenderer::PaletteIndexLines[dst_y]][0]; \
        for (int dst_x = dst_x1, src_x = src_x2; dst_x < dst_x2; dst_x++, src_x--) { \
            placePixelMacro(pixelFunction) \
        } \
        dst_strideY += dstStride; src_strideY -= srcStride; \
    }

    #define BLENDFLAGS(flipMacro, placePixelMacro) \
        switch (blendFlag) { \
            case 0: \
                flipMacro(PixelNoFiltSetOpaque, placePixelMacro); \
                break; \
            case 1: \
                flipMacro(PixelNoFiltSetTransparent, placePixelMacro); \
                break; \
            case 2: \
                flipMacro(PixelNoFiltSetAdditive, placePixelMacro); \
                break; \
            case 3: \
                flipMacro(PixelNoFiltSetSubtract, placePixelMacro); \
                break; \
        }

    register Uint32 color;
    register Uint32* index;
    register int dst_strideY, src_strideY;
    register int* multTableAt = &MultTable[opacity << 8];
    register int* multSubTableAt = &MultSubTable[opacity << 8];
    if (Graphics::UsePalettes && texture->Paletted) {
        switch (flipFlag) {
            case 0:
                dst_strideY = dst_y1 * dstStride;
                src_strideY = src_y1 * srcStride;
                BLENDFLAGS(DRAW_NOFLIP, DRAW_PLACEPIXEL_PAL);
                break;
            case 1:
                dst_strideY = dst_y1 * dstStride;
                src_strideY = src_y1 * srcStride;
                BLENDFLAGS(DRAW_FLIPX, DRAW_PLACEPIXEL_PAL);
                break;
            case 2:
                dst_strideY = dst_y1 * dstStride;
                src_strideY = src_y2 * srcStride;
                BLENDFLAGS(DRAW_FLIPY, DRAW_PLACEPIXEL_PAL);
                break;
            case 3:
                dst_strideY = dst_y1 * dstStride;
                src_strideY = src_y2 * srcStride;
                BLENDFLAGS(DRAW_FLIPXY, DRAW_PLACEPIXEL_PAL);
                break;
        }
    }
    else {
        switch (flipFlag) {
            case 0:
                dst_strideY = dst_y1 * dstStride;
                src_strideY = src_y1 * srcStride;
                BLENDFLAGS(DRAW_NOFLIP, DRAW_PLACEPIXEL);
                break;
            case 1:
                dst_strideY = dst_y1 * dstStride;
                src_strideY = src_y1 * srcStride;
                BLENDFLAGS(DRAW_FLIPX, DRAW_PLACEPIXEL);
                break;
            case 2:
                dst_strideY = dst_y1 * dstStride;
                src_strideY = src_y2 * srcStride;
                BLENDFLAGS(DRAW_FLIPY, DRAW_PLACEPIXEL);
                break;
            case 3:
                dst_strideY = dst_y1 * dstStride;
                src_strideY = src_y2 * srcStride;
                BLENDFLAGS(DRAW_FLIPXY, DRAW_PLACEPIXEL);
                break;
        }
    }

    #undef DRAW_PLACEPIXEL
    #undef DRAW_PLACEPIXEL_PAL
    #undef DRAW_NOFLIP
    #undef DRAW_FLIPX
    #undef DRAW_FLIPY
    #undef DRAW_FLIPXY
    #undef BLENDFLAGS
}
void DrawSpriteImageTransformed(Texture* texture, int x, int y, int offx, int offy, int w, int h, int sx, int sy, int sw, int sh, int flipFlag, int rotation, int blendFlag, int opacity) {
    register Uint32* srcPx = (Uint32*)texture->Pixels;
    register Uint32  srcStride = texture->Width;
    register Uint32* srcPxLine;

    register Uint32* dstPx = (Uint32*)Graphics::CurrentRenderTarget->Pixels;
    register Uint32  dstStride = Graphics::CurrentRenderTarget->Width;
    register Uint32* dstPxLine;

    register int src_x;
    register int src_y;
    register int src_x1 = sx;
    register int src_y1 = sy;
    register int src_x2 = sx + sw - 1;
    register int src_y2 = sy + sh - 1;

    register int cos = Cos0x200[rotation];
    register int sin = Sin0x200[rotation];
    register int rcos = Cos0x200[(0x200 - rotation + 0x200) & 0x1FF];
    register int rsin = Sin0x200[(0x200 - rotation + 0x200) & 0x1FF];

    register int _x1 = offx;
    register int _y1 = offy;
    register int _x2 = offx + w;
    register int _y2 = offy + h;

    switch (flipFlag) {
        case 1: _x1 = -offx - w, _x2 = -offx; break;
        case 2: _y1 = -offy - h, _y2 = -offy; break;
        case 3:
            _x1 = -offx - w, _x2 = -offx;
            _y1 = -offy - h, _y2 = -offy;
    }

    register int dst_x1 = _x1;
    register int dst_y1 = _y1;
    register int dst_x2 = _x2;
    register int dst_y2 = _y2;

    if (Alpha != 0 && blendFlag == 0)
        blendFlag = 1;
    if (!Graphics::TextureBlend)
        blendFlag = 0;

    #define SET_MIN(a, b) if (a > b) a = b;
    #define SET_MAX(a, b) if (a < b) a = b;

    register int px, py, cx, cy;

    py = _y1;
    {
        px = _x1;
        cx = (px * cos - py * sin); SET_MIN(dst_x1, cx); SET_MAX(dst_x2, cx);
        cy = (px * sin + py * cos); SET_MIN(dst_y1, cy); SET_MAX(dst_y2, cy);

        px = _x2;
        cx = (px * cos - py * sin); SET_MIN(dst_x1, cx); SET_MAX(dst_x2, cx);
        cy = (px * sin + py * cos); SET_MIN(dst_y1, cy); SET_MAX(dst_y2, cy);
    }

    py = _y2;
    {
        px = _x1;
        cx = (px * cos - py * sin); SET_MIN(dst_x1, cx); SET_MAX(dst_x2, cx);
        cy = (px * sin + py * cos); SET_MIN(dst_y1, cy); SET_MAX(dst_y2, cy);

        px = _x2;
        cx = (px * cos - py * sin); SET_MIN(dst_x1, cx); SET_MAX(dst_x2, cx);
        cy = (px * sin + py * cos); SET_MIN(dst_y1, cy); SET_MAX(dst_y2, cy);
    }

    #undef SET_MIN
    #undef SET_MAX

    dst_x1 >>= 9;
    dst_y1 >>= 9;
    dst_x2 >>= 9;
    dst_y2 >>= 9;

    dst_x1 += x;
    dst_y1 += y;
    dst_x2 += x + 1;
    dst_y2 += y + 1;

    if (Graphics::CurrentClip.Enabled) {
        register int
            clip_x1 = Graphics::CurrentClip.X,
            clip_y1 = Graphics::CurrentClip.Y,
            clip_x2 = Graphics::CurrentClip.X + Graphics::CurrentClip.Width,
            clip_y2 = Graphics::CurrentClip.Y + Graphics::CurrentClip.Height;

        if (dst_x1 < clip_x1)
            dst_x1 = clip_x1;
        if (dst_y1 < clip_y1)
            dst_y1 = clip_y1;
        if (dst_x2 > clip_x2)
            dst_x2 = clip_x2;
        if (dst_y2 > clip_y2)
            dst_y2 = clip_y2;
    }
    else {
        register int
            clip_x2 = (int)Graphics::CurrentRenderTarget->Width,
            clip_y2 = (int)Graphics::CurrentRenderTarget->Height;
        if (dst_x1 < 0)
            dst_x1 = 0;
        if (dst_y1 < 0)
            dst_y1 = 0;
        if (dst_x2 > clip_x2)
            dst_x2 = clip_x2;
        if (dst_y2 > clip_y2)
            dst_y2 = clip_y2;
    }

    if (dst_x1 >= dst_x2)
        return;
    if (dst_y1 >= dst_y2)
        return;

    #define DRAW_PLACEPIXEL(pixelFunction) \
        if ((color = srcPx[src_x + src_strideY]) & 0xFF000000U) \
            pixelFunction(&color, &dstPxLine[dst_x], opacity, multTableAt, multSubTableAt);
    #define DRAW_PLACEPIXEL_PAL(pixelFunction) \
        if ((color = srcPx[src_x + src_strideY])) \
            pixelFunction(&index[color], &dstPxLine[dst_x], opacity, multTableAt, multSubTableAt);

    #define DRAW_NOFLIP(pixelFunction, placePixelMacro) for (int dst_y = dst_y1, i_y = dst_y1 - y; dst_y < dst_y2; dst_y++, i_y++) { \
        i_y_rsin = -i_y * rsin; \
        i_y_rcos =  i_y * rcos; \
        dstPxLine = dstPx + dst_strideY; \
        index = &SoftwareRenderer::PaletteColors[SoftwareRenderer::PaletteIndexLines[dst_y]][0]; \
        for (int dst_x = dst_x1, i_x = dst_x1 - x; dst_x < dst_x2; dst_x++, i_x++) { \
            src_x = (i_x * rcos + i_y_rsin) >> 9; \
            src_y = (i_x * rsin + i_y_rcos) >> 9; \
            if (src_x >= _x1 && src_y >= _y1 && \
                src_x <  _x2 && src_y <  _y2) { \
                src_x       = (src_x1 + (src_x - _x1) * sw / w); \
                src_strideY = (src_y1 + (src_y - _y1) * sh / h) * srcStride; \
                placePixelMacro(pixelFunction); \
            } \
        } \
        dst_strideY += dstStride; \
    }
    #define DRAW_FLIPX(pixelFunction, placePixelMacro) for (int dst_y = dst_y1, i_y = dst_y1 - y; dst_y < dst_y2; dst_y++, i_y++) { \
        i_y_rsin = -i_y * rsin; \
        i_y_rcos =  i_y * rcos; \
        dstPxLine = dstPx + dst_strideY; \
        index = &SoftwareRenderer::PaletteColors[SoftwareRenderer::PaletteIndexLines[dst_y]][0]; \
        for (int dst_x = dst_x1, i_x = dst_x1 - x; dst_x < dst_x2; dst_x++, i_x++) { \
            src_x = (i_x * rcos + i_y_rsin) >> 9; \
            src_y = (i_x * rsin + i_y_rcos) >> 9; \
            if (src_x >= _x1 && src_y >= _y1 && \
                src_x <  _x2 && src_y <  _y2) { \
                src_x       = (src_x2 - (src_x - _x1) * sw / w); \
                src_strideY = (src_y1 + (src_y - _y1) * sh / h) * srcStride; \
                placePixelMacro(pixelFunction); \
            } \
        } \
        dst_strideY += dstStride; \
    }
    #define DRAW_FLIPY(pixelFunction, placePixelMacro) for (int dst_y = dst_y1, i_y = dst_y1 - y; dst_y < dst_y2; dst_y++, i_y++) { \
        i_y_rsin = -i_y * rsin; \
        i_y_rcos =  i_y * rcos; \
        dstPxLine = dstPx + dst_strideY; \
        index = &SoftwareRenderer::PaletteColors[SoftwareRenderer::PaletteIndexLines[dst_y]][0]; \
        for (int dst_x = dst_x1, i_x = dst_x1 - x; dst_x < dst_x2; dst_x++, i_x++) { \
            src_x = (i_x * rcos + i_y_rsin) >> 9; \
            src_y = (i_x * rsin + i_y_rcos) >> 9; \
            if (src_x >= _x1 && src_y >= _y1 && \
                src_x <  _x2 && src_y <  _y2) { \
                src_x       = (src_x1 + (src_x - _x1) * sw / w); \
                src_strideY = (src_y2 - (src_y - _y1) * sh / h) * srcStride; \
                placePixelMacro(pixelFunction); \
            } \
        } \
        dst_strideY += dstStride; \
    }
    #define DRAW_FLIPXY(pixelFunction, placePixelMacro) for (int dst_y = dst_y1, i_y = dst_y1 - y; dst_y < dst_y2; dst_y++, i_y++) { \
        i_y_rsin = -i_y * rsin; \
        i_y_rcos =  i_y * rcos; \
        dstPxLine = dstPx + dst_strideY; \
        index = &SoftwareRenderer::PaletteColors[SoftwareRenderer::PaletteIndexLines[dst_y]][0]; \
        for (int dst_x = dst_x1, i_x = dst_x1 - x; dst_x < dst_x2; dst_x++, i_x++) { \
            src_x = (i_x * rcos + i_y_rsin) >> 9; \
            src_y = (i_x * rsin + i_y_rcos) >> 9; \
            if (src_x >= _x1 && src_y >= _y1 && \
                src_x <  _x2 && src_y <  _y2) { \
                src_x       = (src_x2 - (src_x - _x1) * sw / w); \
                src_strideY = (src_y2 - (src_y - _y1) * sh / h) * srcStride; \
                placePixelMacro(pixelFunction); \
            } \
        } \
        dst_strideY += dstStride; \
    }

    #define BLENDFLAGS(flipMacro, placePixelMacro) \
        switch (blendFlag) { \
            case 0: \
                flipMacro(PixelNoFiltSetOpaque, placePixelMacro); \
                break; \
            case 1: \
                flipMacro(PixelNoFiltSetTransparent, placePixelMacro); \
                break; \
            case 2: \
                flipMacro(PixelNoFiltSetAdditive, placePixelMacro); \
                break; \
            case 3: \
                flipMacro(PixelNoFiltSetSubtract, placePixelMacro); \
                break; \
        }

    register Uint32 color;
    register Uint32* index;
    register int i_y_rsin, i_y_rcos;
    register int dst_strideY, src_strideY;
    register int* multTableAt = &MultTable[opacity << 8];
    register int* multSubTableAt = &MultSubTable[opacity << 8];
    if (Graphics::UsePalettes && texture->Paletted) {
        switch (flipFlag) {
            case 0:
                dst_strideY = dst_y1 * dstStride;
                BLENDFLAGS(DRAW_NOFLIP, DRAW_PLACEPIXEL_PAL);
                break;
            case 1:
                dst_strideY = dst_y1 * dstStride;
                BLENDFLAGS(DRAW_FLIPX, DRAW_PLACEPIXEL_PAL);
                break;
            case 2:
                dst_strideY = dst_y1 * dstStride;
                BLENDFLAGS(DRAW_FLIPY, DRAW_PLACEPIXEL_PAL);
                break;
            case 3:
                dst_strideY = dst_y1 * dstStride;
                BLENDFLAGS(DRAW_FLIPXY, DRAW_PLACEPIXEL_PAL);
                break;
        }
    }
    else {
        switch (flipFlag) {
            case 0:
                dst_strideY = dst_y1 * dstStride;
                BLENDFLAGS(DRAW_NOFLIP, DRAW_PLACEPIXEL);
                break;
            case 1:
                dst_strideY = dst_y1 * dstStride;
                BLENDFLAGS(DRAW_FLIPX, DRAW_PLACEPIXEL);
                break;
            case 2:
                dst_strideY = dst_y1 * dstStride;
                BLENDFLAGS(DRAW_FLIPY, DRAW_PLACEPIXEL);
                break;
            case 3:
                dst_strideY = dst_y1 * dstStride;
                BLENDFLAGS(DRAW_FLIPXY, DRAW_PLACEPIXEL);
                break;
        }
    }

    #undef DRAW_PLACEPIXEL
    #undef DRAW_PLACEPIXEL_PAL
    #undef DRAW_NOFLIP
    #undef DRAW_FLIPX
    #undef DRAW_FLIPY
    #undef DRAW_FLIPXY
    #undef BLENDFLAGS
}

PUBLIC STATIC void     SoftwareRenderer::DrawTexture(Texture* texture, float sx, float sy, float sw, float sh, float x, float y, float w, float h) {
    View* currentView = &Scene::Views[Scene::ViewCurrent];
    int cx = (int)std::floor(currentView->X);
    int cy = (int)std::floor(currentView->Y);

    Matrix4x4* out = Graphics::ModelViewMatrix.top();
    x += out->Values[12];
    y += out->Values[13];
    x -= cx;
    y -= cy;

    DrawSpriteImage(texture,
        x, y, (int)sw, (int)sh, (int)sx, (int)sy, 0, BlendFlag, Alpha);
}
PUBLIC STATIC void     SoftwareRenderer::DrawSprite(ISprite* sprite, int animation, int frame, int x, int y, bool flipX, bool flipY, float scaleW, float scaleH, float rotation) {
    if (Graphics::SpriteRangeCheck(sprite, animation, frame)) return;

    AnimFrame frameStr = sprite->Animations[animation].Frames[frame];
    Texture* texture = sprite->Spritesheets[frameStr.SheetNumber];

    View* currentView = &Scene::Views[Scene::ViewCurrent];
    int cx = (int)std::floor(currentView->X);
    int cy = (int)std::floor(currentView->Y);

    Matrix4x4* out = Graphics::ModelViewMatrix.top();
    x += out->Values[12];
    y += out->Values[13];
    x -= cx;
    y -= cy;

    int flipFlag = (int)flipX | ((int)flipY << 1);
    if (rotation != 0.0f || scaleW != 1.0f || scaleH != 1.0f) {
        int rot = (int)(rotation * 0x100 / M_PI) & 0x1FF;
        DrawSpriteImageTransformed(texture,
            x, y,
            frameStr.OffsetX * scaleW, frameStr.OffsetY * scaleH,
            frameStr.Width * scaleW, frameStr.Height * scaleH,

            frameStr.X, frameStr.Y,
            frameStr.Width, frameStr.Height,
            flipFlag, rot,
            BlendFlag, Alpha);
        return;
    }
    switch (flipFlag) {
        case 0:
            DrawSpriteImage(texture,
                x + frameStr.OffsetX,
                y + frameStr.OffsetY,
                frameStr.Width, frameStr.Height, frameStr.X, frameStr.Y, flipFlag, BlendFlag, Alpha);
            break;
        case 1:
            DrawSpriteImage(texture,
                x - frameStr.OffsetX - frameStr.Width,
                y + frameStr.OffsetY,
                frameStr.Width, frameStr.Height, frameStr.X, frameStr.Y, flipFlag, BlendFlag, Alpha);
            break;
        case 2:
            DrawSpriteImage(texture,
                x + frameStr.OffsetX,
                y - frameStr.OffsetY - frameStr.Height,
                frameStr.Width, frameStr.Height, frameStr.X, frameStr.Y, flipFlag, BlendFlag, Alpha);
            break;
        case 3:
            DrawSpriteImage(texture,
                x - frameStr.OffsetX - frameStr.Width,
                y - frameStr.OffsetY - frameStr.Height,
                frameStr.Width, frameStr.Height, frameStr.X, frameStr.Y, flipFlag, BlendFlag, Alpha);
            break;
    }
}
PUBLIC STATIC void     SoftwareRenderer::DrawSpritePart(ISprite* sprite, int animation, int frame, int sx, int sy, int sw, int sh, int x, int y, bool flipX, bool flipY, float scaleW, float scaleH, float rotation) {
	if (Graphics::SpriteRangeCheck(sprite, animation, frame)) return;

	/*

	bool AltPal = false;
	Uint32 PixelIndex, *Pal = sprite->Palette;
	AnimFrame animframe = sprite->Animations[animation].Frames[frame];

	srcw = Math::Min(srcw, animframe.Width- srcx);
	srch = Math::Min(srch, animframe.H - srcy);

	int DX = 0, DY = 0, PX, PY, size = srcw * srch;
	int CenterX = x, CenterY = y, RealCenterX = animframe.OffX, RealCenterY = animframe.OffY;
	int SrcX = animframe.X + srcx, SrcY = animframe.Y + srcy, Width = srcw - 1, Height = srch - 1;

	if (flipX)
		RealCenterX = -RealCenterX - Width - 1;
	if (flipY)
		RealCenterY = -RealCenterY - Height - 1;

	if (!AltPal && CenterY + RealCenterY >= WaterPaletteStartLine)
		AltPal = true, Pal = sprite->PaletteAlt;

	if (CenterX + Width + RealCenterX < Clip[0]) return;
	if (CenterY + Height + RealCenterY < Clip[1]) return;
	if (CenterX + RealCenterX >= Clip[2]) return;
	if (CenterY + RealCenterY >= Clip[3]) return;

	int FX, FY;
	for (int D = 0; D < size; D++) {
		PX = DX;
		PY = DY;
		if (flipX)
			PX = Width - PX;
		if (flipY)
			PY = Height - PY;

		FX = CenterX + DX + RealCenterX;
		FY = CenterY + DY + RealCenterY;
		if (FX <  Clip[0]) goto CONTINUE;
		if (FY <  Clip[1]) goto CONTINUE;
		if (FX >= Clip[2]) goto CONTINUE;
		if (FY >= Clip[3]) return;

		PixelIndex = GetPixelIndex(sprite, SrcX + PX, SrcY + PY);
		if (sprite->PaletteCount) {
			if (PixelIndex != sprite->TransparentColorIndex)
				SetPixelFunction(FX, FY, GetPixelOutputColor(Pal[PixelIndex]));
				// SetPixelFunction(FX, FY, Pal[PixelIndex]);
		}
		else if (PixelIndex & 0xFF000000U) {
			SetPixelFunction(FX, FY, GetPixelOutputColor(PixelIndex));
			// SetPixelFunction(FX, FY, PixelIndex);
		}

		CONTINUE:

		DX++;
		if (DX > Width) {
			DX = 0, DY++;

			if (!AltPal && CenterY + DY + RealCenterY >= WaterPaletteStartLine)
				AltPal = true, Pal = sprite->PaletteAlt;
		}
	}
	//*/

    AnimFrame frameStr = sprite->Animations[animation].Frames[frame];
    Texture* texture = sprite->Spritesheets[frameStr.SheetNumber];

    View* currentView = &Scene::Views[Scene::ViewCurrent];
    int cx = (int)std::floor(currentView->X);
    int cy = (int)std::floor(currentView->Y);

    Matrix4x4* out = Graphics::ModelViewMatrix.top();
    x += out->Values[12];
    y += out->Values[13];
    x -= cx;
    y -= cy;

    if (sw >= frameStr.Width - sx)
        sw  = frameStr.Width - sx;
    if (sh >= frameStr.Height - sy)
        sh  = frameStr.Height - sy;

    int flipFlag = flipX | (flipY << 1);
    switch (flipFlag) {
        case 0:
            DrawSpriteImage(texture,
                x + frameStr.OffsetX + sx,
                y + frameStr.OffsetY + sy,
                sw, sh, frameStr.X + sx, frameStr.Y + sy, flipFlag, BlendFlag, Alpha);
            break;
        case 1:
            DrawSpriteImage(texture,
                x - frameStr.OffsetX - sw - sx,
                y + frameStr.OffsetY + sy,
                sw, sh, frameStr.X + sx, frameStr.Y + sy, flipFlag, BlendFlag, Alpha);
            break;
        case 2:
            DrawSpriteImage(texture,
                x + frameStr.OffsetX + sx,
                y - frameStr.OffsetY - sh - sy,
                sw, sh, frameStr.X + sx, frameStr.Y + sy, flipFlag, BlendFlag, Alpha);
            break;
        case 3:
            DrawSpriteImage(texture,
                x - frameStr.OffsetX - sw - sx,
                y - frameStr.OffsetY - sh - sy,
                sw, sh, frameStr.X + sx, frameStr.Y + sy, flipFlag, BlendFlag, Alpha);
            break;
    }
}

PUBLIC STATIC void     SoftwareRenderer::MakeFrameBufferID(ISprite* sprite, AnimFrame* frame) {
    frame->ID = 0;
}
