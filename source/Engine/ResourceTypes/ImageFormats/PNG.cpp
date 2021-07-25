#if INTERFACE
#include <Engine/Includes/Standard.h>
#include <Engine/ResourceTypes/ImageFormats/ImageFormat.h>
#include <Engine/IO/Stream.h>

class PNG : public ImageFormat {
public:

};
#endif

#include <Engine/ResourceTypes/ImageFormats/PNG.h>

#include <Engine/Application.h>
#include <Engine/Graphics.h>
#include <Engine/Diagnostics/Log.h>
#include <Engine/Diagnostics/Clock.h>
#include <Engine/Diagnostics/Memory.h>
#include <Engine/IO/FileStream.h>
#include <Engine/IO/MemoryStream.h>
#include <Engine/IO/ResourceStream.h>

// #ifdef USING_IMAGEIO
// #endif

#ifdef USING_LIBPNG

#include USING_LIBPNG_HEADER

void png_read_fn(png_structp ctx, png_bytep area, png_size_t size) {
    Stream* stream = (Stream*)png_get_io_ptr(ctx);
    stream->ReadBytes(area, size);
}
#else
#define STB_IMAGE_IMPLEMENTATION
#include <Libraries/stb_image.h>
#endif



PUBLIC STATIC  PNG*   PNG::Load(const char* filename) {
#ifdef USING_LIBPNG
    PNG* png = new PNG;
    Stream* stream = NULL;
    // size_t fileSize;
    // void*  fileBuffer = NULL;
    // SDL_Surface* temp = NULL,* tempOld = NULL;

    // PNG variables
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    Uint32 width, height;
    int bit_depth, color_type, interlace_type, num_channels;
    Uint32* pixelData = NULL;
    png_bytep* row_pointers = NULL;
    Uint32 Rmask, Gmask, Bmask, Amask;
    bool doConvert;

    if (strncmp(filename, "file://", 7) == 0)
        stream = FileStream::New(filename + 7, FileStream::READ_ACCESS);
    else
        stream = ResourceStream::New(filename);
    if (!stream) {
        Log::Print(Log::LOG_ERROR, "Could not open file '%s'!", filename);
        goto PNG_Load_FAIL;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL){
        Log::Print(Log::LOG_ERROR, "Couldn't create PNG read struct!");
        goto PNG_Load_FAIL;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        Log::Print(Log::LOG_ERROR, "Couldn't create PNG information struct!");
        goto PNG_Load_FAIL;
    }

    if (setjmp(*png_set_longjmp_fn(png_ptr, longjmp, sizeof(jmp_buf)))) {
        Log::Print(Log::LOG_ERROR, "Couldn't read PNG file!");
        goto PNG_Load_FAIL;
    }

    png_set_read_fn(png_ptr, stream, png_read_fn);
    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);
    png_set_strip_16(png_ptr);
    png_set_interlace_handling(png_ptr);
    png_set_packing(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY)
        png_set_expand(png_ptr);


    png_read_update_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);

    num_channels = png_get_channels(png_ptr, info_ptr);

    png->Width = width;
    png->Height = height;
    png->Data = (Uint32*)Memory::TrackedMalloc("PNG::Data", png->Width * png->Height * sizeof(Uint32));
    if (Graphics::UsePalettes) {
        png->Colors = (Uint32*)Memory::TrackedMalloc("PNG::Colors", 0x100 * sizeof(Uint32));
    }
    png->TransparentColorIndex = -1;

    pixelData = (Uint32*)malloc(png->Width * png->Height * sizeof(Uint32));

    row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * png->Height);
    if (!row_pointers) {
        goto PNG_Load_FAIL;
    }

    for (Uint32 row = 0, pitch = png->Width * num_channels; row < png->Height; row++) {
        row_pointers[row] = (png_bytep)((Uint8*)pixelData + row * pitch);
    }

    png_read_image(png_ptr, row_pointers);

    doConvert = false;
    if (Graphics::PreferredPixelFormat == SDL_PIXELFORMAT_ABGR8888) {
        Amask = (num_channels == 4) ? 0xFF000000 : 0;
        Bmask = 0x00FF0000;
        Gmask = 0x0000FF00;
        Rmask = 0x000000FF;
        if (num_channels != 4)
            doConvert = true;
    }
    else if (Graphics::PreferredPixelFormat == SDL_PIXELFORMAT_ARGB8888) {
        Amask = (num_channels == 4) ? 0xFF000000 : 0;
        Rmask = 0x00FF0000;
        Gmask = 0x0000FF00;
        Bmask = 0x000000FF;
        doConvert = true;
    }

    if (doConvert) {
        Uint8* px;
        Uint32 a, b, g, r;
        int pc = 0, size = png->Width * png->Height;
        if (Amask) {
            for (Uint32* i = pixelData; pc < size; i++, pc++) {
                px = (Uint8*)i;
                r = px[0] | px[0] << 8 | px[0] << 16 | px[0] << 24;
                g = px[1] | px[1] << 8 | px[1] << 16 | px[1] << 24;
                b = px[2] | px[2] << 8 | px[2] << 16 | px[2] << 24;
                a = px[3] | px[3] << 8 | px[3] << 16 | px[3] << 24;
                png->Data[pc] = (r & Rmask) | (g & Gmask) | (b & Bmask) | (a & Amask);
            }
        }
        else {
            for (Uint8* i = (Uint8*)pixelData; pc < size; i += 3, pc++) {
                px = (Uint8*)i;
                r = px[0] | px[0] << 8 | px[0] << 16 | px[0] << 24;
                g = px[1] | px[1] << 8 | px[1] << 16 | px[1] << 24;
                b = px[2] | px[2] << 8 | px[2] << 16 | px[2] << 24;
                png->Data[pc] = (r & Rmask) | (g & Gmask) | (b & Bmask) | 0xFF000000;
            }
        }
    }
    else {
        memcpy(png->Data, pixelData, png->Width * png->Height * sizeof(Uint32));
    }
    free(pixelData);

    png->Paletted = false;

    goto PNG_Load_Success;

    PNG_Load_FAIL:
        delete png;
        png = NULL;

    PNG_Load_Success:
        if (png_ptr)
            png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : (png_infopp)NULL, (png_infopp)NULL);
        if (row_pointers)
            free(row_pointers);
        if (stream)
            stream->Close();
        return png;
#else
    PNG* png = new PNG;
    Stream* stream = NULL;
    Uint32* pixelData = NULL;
    int num_channels;
    int width, height;
    Uint32 Rmask, Gmask, Bmask, Amask;
    bool doConvert;
    Uint8* buffer = NULL;
    size_t buffer_len = 0;

    if (strncmp(filename, "file://", 7) == 0)
        stream = FileStream::New(filename + 7, FileStream::READ_ACCESS);
    else
        stream = ResourceStream::New(filename);
    if (!stream) {
        Log::Print(Log::LOG_ERROR, "Could not open file '%s'!", filename);
        goto PNG_Load_FAIL;
    }

    buffer_len = stream->Length();
    buffer = (Uint8*)malloc(buffer_len);
    stream->ReadBytes(buffer, buffer_len);

    pixelData = (Uint32*)stbi_load_from_memory(buffer, buffer_len, &width, &height, &num_channels, STBI_rgb_alpha);
    if (!pixelData) {
        Log::Print(Log::LOG_ERROR, "stbi_load failed: %s", stbi_failure_reason());
        goto PNG_Load_FAIL;
    }

    png->Width = width;
    png->Height = height;
    png->Data = (Uint32*)Memory::TrackedMalloc("PNG::Data", png->Width * png->Height * sizeof(Uint32));
    if (Graphics::UsePalettes) {
        png->Colors = (Uint32*)Memory::TrackedMalloc("PNG::Colors", 0x100 * sizeof(Uint32));
    }
    png->TransparentColorIndex = -1;

    doConvert = false;
    if (Graphics::PreferredPixelFormat == SDL_PIXELFORMAT_ABGR8888) {
        Amask = (num_channels == 4) ? 0xFF000000 : 0;
        Bmask = 0x00FF0000;
        Gmask = 0x0000FF00;
        Rmask = 0x000000FF;
        if (num_channels != 4)
            doConvert = true;
    }
    else if (Graphics::PreferredPixelFormat == SDL_PIXELFORMAT_ARGB8888) {
        Amask = (num_channels == 4) ? 0xFF000000 : 0;
        Rmask = 0x00FF0000;
        Gmask = 0x0000FF00;
        Bmask = 0x000000FF;
        doConvert = true;
    }

    if (doConvert) {
        Uint8* px;
        Uint32 a, b, g, r;
        int pc = 0, size = png->Width * png->Height;
        if (Amask) {
            for (Uint32* i = pixelData; pc < size; i++, pc++) {
                px = (Uint8*)i;
                r = px[0] | px[0] << 8 | px[0] << 16 | px[0] << 24;
                g = px[1] | px[1] << 8 | px[1] << 16 | px[1] << 24;
                b = px[2] | px[2] << 8 | px[2] << 16 | px[2] << 24;
                a = px[3] | px[3] << 8 | px[3] << 16 | px[3] << 24;
                png->Data[pc] = (r & Rmask) | (g & Gmask) | (b & Bmask) | (a & Amask);
            }
        }
        else {
            for (Uint8* i = (Uint8*)pixelData; pc < size; i += 3, pc++) {
                px = (Uint8*)i;
                r = px[0] | px[0] << 8 | px[0] << 16 | px[0] << 24;
                g = px[1] | px[1] << 8 | px[1] << 16 | px[1] << 24;
                b = px[2] | px[2] << 8 | px[2] << 16 | px[2] << 24;
                png->Data[pc] = (r & Rmask) | (g & Gmask) | (b & Bmask) | 0xFF000000;
            }
        }
    }
    else {
        memcpy(png->Data, pixelData, png->Width * png->Height * sizeof(Uint32));
    }

    png->Paletted = false;




    goto PNG_Load_Success;

PNG_Load_FAIL:
    delete png;
    png = NULL;

PNG_Load_Success:
    if (buffer)
        free(buffer);
    if (pixelData)
        stbi_image_free(pixelData);
    if (stream)
        stream->Close();
    return png;
#endif
	return NULL;
}
PUBLIC STATIC  bool   PNG::Save(PNG* png, const char* filename) {
    return png->Save(filename);
}

PUBLIC        bool    PNG::Save(const char* filename) {
    Stream* stream = FileStream::New(filename, FileStream::WRITE_ACCESS);
    if (!stream)
        return false;

    stream->Close();
    return true;
}

PUBLIC                PNG::~PNG() {

}
