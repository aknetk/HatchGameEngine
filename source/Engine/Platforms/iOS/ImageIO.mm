/*
 *  IMG_ImageIO.c
 *  SDL_image
 *
 *  Created by Eric Wing on 1/1/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <Engine/IO/Stream.h>
#include "SDL.h"

// Used because CGDataProviderCreate became deprecated in 10.5
#include <AvailabilityMacros.h>
#include <TargetConditionals.h>
#include <Foundation/Foundation.h>

#if (TARGET_OS_IPHONE == 1) || (TARGET_IPHONE_SIMULATOR == 1)
    #ifdef ALLOW_UIIMAGE_FALLBACK
        #define USE_UIIMAGE_BACKEND() ([UIImage instancesRespondToSelector:@selector(initWithCGImage:scale:orientation:)] == NO)
    #else
        #define USE_UIIMAGE_BACKEND() (Internal_checkImageIOisAvailable())
    #endif

    #import <MobileCoreServices/MobileCoreServices.h> // for UTCoreTypes.h
    #import <ImageIO/ImageIO.h>
    #import <UIKit/UIImage.h>
#else
    // For ImageIO framework and also LaunchServices framework (for UTIs)
    #include <ApplicationServices/ApplicationServices.h>
#endif

#if !defined(ALLOW_UIIMAGE_FALLBACK) && ((TARGET_OS_IPHONE == 1) || (TARGET_IPHONE_SIMULATOR == 1))
    static int Internal_checkImageIOisAvailable() {
        // just check if we are running on ios 4 or more, else throw exception
        if ([UIImage instancesRespondToSelector:@selector(initWithCGImage:scale:orientation:)])
            return 0;

        [NSException raise:@"UIImage fallback not enabled at compile time"
                    format:@"ImageIO is not available on your platform, please recompile SDL_Image with ALLOW_UIIMAGE_FALLBACK."];
        return -1;
    }
#endif

// This callback reads some bytes from an SDL_rwops and copies it
// to a Quartz buffer (supplied by Apple framework).
static size_t HatchProviderGetBytesCallback(void* rwops_userdata, void* quartz_buffer, size_t the_count) {
    return (size_t)((Stream*)rwops_userdata)->ReadBytes(quartz_buffer, the_count);
}
// This callback is triggered when the data provider is released
// so you can clean up any resources.
static void HatchProviderReleaseInfoCallback(void* rwops_userdata) {
    // What should I put here?
    // I think the user and SDL_RWops controls closing, so I don't do anything.
}
static void HatchProviderRewindCallback(void* rwops_userdata) {
    ((Stream*)rwops_userdata)->Seek(0);
}

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1050 // CGDataProviderCreateSequential was introduced in 10.5; CGDataProviderCreate is deprecated
    off_t HatchProviderSkipForwardBytesCallback(void* rwops_userdata, off_t the_count) {
        off_t start_position = ((Stream*)rwops_userdata)->Position();
        ((Stream*)rwops_userdata)->Skip(the_count);
        off_t end_position = ((Stream*)rwops_userdata)->Position();
        return (end_position - start_position);
    }
#else // CGDataProviderCreate was deprecated in 10.5
    static void HatchProviderSkipBytesCallback(void* rwops_userdata, size_t the_count) {
        ((Stream*)rwops_userdata)->Skip(the_count);
    }
#endif

// This creates a CGImageSourceRef which is a handle to an image that can
// be used to examine information about the image or load the actual image data.
static CGImageSourceRef Create_CGImageSource_FromStream(Stream* rw_ops, CFDictionaryRef hints_and_options) {
    CGImageSourceRef source_ref;

    // Similar to SDL_RWops, Apple has their own callbacks for dealing with data streams.
    #if MAC_OS_X_VERSION_MAX_ALLOWED >= 1050 // CGDataProviderCreateSequential was introduced in 10.5; CGDataProviderCreate is deprecated
        CGDataProviderSequentialCallbacks provider_callbacks = {
            0,
            HatchProviderGetBytesCallback,
            HatchProviderSkipForwardBytesCallback,
            HatchProviderRewindCallback,
            HatchProviderReleaseInfoCallback
        };

        CGDataProviderRef data_provider = CGDataProviderCreateSequential(rw_ops, &provider_callbacks);
    #else // CGDataProviderCreate was deprecated in 10.5
        CGDataProviderCallbacks provider_callbacks = {
            HatchProviderGetBytesCallback,
            HatchProviderSkipBytesCallback,
            HatchProviderRewindCallback,
            HatchProviderReleaseInfoCallback
        };

        CGDataProviderRef data_provider = CGDataProviderCreate(rw_ops, &provider_callbacks);
    #endif
    // Get the CGImageSourceRef.
    // The dictionary can be NULL or contain hints to help ImageIO figure out the image type.
    source_ref = CGImageSourceCreateWithDataProvider(data_provider, hints_and_options);
    CGDataProviderRelease(data_provider);
    return source_ref;
}
// Create a CGImageSourceRef from a file, remember to CFRelease the created source when done.
static CGImageSourceRef Create_CGImageSource_FromFile(const char* the_path) {
    CFURLRef the_url = NULL;
    CGImageSourceRef source_ref = NULL;
    CFStringRef cf_string = NULL;

    /* Create a CFString from a C string */
    cf_string = CFStringCreateWithCString(NULL, the_path, kCFStringEncodingUTF8);
    if (!cf_string) {
        return NULL;
    }

    /* Create a CFURL from a CFString */
    the_url = CFURLCreateWithFileSystemPath(NULL, cf_string, kCFURLPOSIXPathStyle, false);

    /* Don't need the CFString any more (error or not) */
    CFRelease(cf_string);

    if (!the_url) {
        return NULL;
    }


    source_ref = CGImageSourceCreateWithURL(the_url, NULL);
    /* Don't need the URL any more (error or not) */
    CFRelease(the_url);

    return source_ref;
}
// Build image from CGImageSource
static CGImageRef       Create_CGImage_FromCGImageSource(CGImageSourceRef image_source) {
    CGImageRef image_ref = NULL;

    if (image_source == NULL) {
        return NULL;
    }

    // Get the first item in the image source (some image formats may
    // contain multiple items).
    image_ref = CGImageSourceCreateImageAtIndex(image_source, 0, NULL);
    if (image_ref == NULL) {
        // IMG_SetError("CGImageSourceCreateImageAtIndex() failed");
    }
    return image_ref;
}
static CFDictionaryRef  Create_HintDictionary(CFStringRef uti_string_hint) {
    CFDictionaryRef hint_dictionary = NULL;

    if (uti_string_hint != NULL) {
        // Do a bunch of work to setup a CFDictionary containing the jpeg compression properties.
        CFStringRef the_keys[1];
        CFStringRef the_values[1];

        the_keys[0] = kCGImageSourceTypeIdentifierHint;
        the_values[0] = uti_string_hint;

        // kCFTypeDictionaryKeyCallBacks or kCFCopyStringDictionaryKeyCallBacks?
        hint_dictionary = CFDictionaryCreate(NULL, (const void**)&the_keys, (const void**)&the_values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    }
    return hint_dictionary;
}

// Once we have our image, we need to get it into an SDL_Surface
static SDL_Surface* Create_SDL_Surface_From_CGImage_RGB(CGImageRef image_ref) {
    /* This code is adapted from Apple's Documentation found here:
     * http://developer.apple.com/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/index.html
     * Listing 9-4††Using a Quartz image as a texture source.
     * Unfortunately, this guide doesn't show what to do about
     * non-RGBA image formats so I'm making the rest up.
     * All this code should be scrutinized.
     */

    size_t w = CGImageGetWidth(image_ref);
    size_t h = CGImageGetHeight(image_ref);
	CGRect rect = {{0, 0}, {static_cast<CGFloat>(w), static_cast<CGFloat>(h)}};

    CGImageAlphaInfo alpha = CGImageGetAlphaInfo(image_ref);
    //size_t bits_per_pixel = CGImageGetBitsPerPixel(image_ref);
    size_t bits_per_component = 8;

    SDL_Surface* surface;
    Uint32 Amask;
    Uint32 Rmask;
    Uint32 Gmask;
    Uint32 Bmask;

    CGContextRef bitmap_context;
    CGBitmapInfo bitmap_info;
    CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();

    if (alpha == kCGImageAlphaNone ||
        alpha == kCGImageAlphaNoneSkipFirst ||
        alpha == kCGImageAlphaNoneSkipLast) {
        bitmap_info = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host; /* XRGB */
        Amask = 0x00000000;
    }
    else {
        /* kCGImageAlphaFirst isn't supported */
        //bitmap_info = kCGImageAlphaFirst | kCGBitmapByteOrder32Host; /* ARGB */
        bitmap_info = kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host; /* ARGB */
        Amask = 0xFF000000;
    }

    Rmask = 0x00FF0000;
    Gmask = 0x0000FF00;
    Bmask = 0x000000FF;

    surface = SDL_CreateRGBSurface(SDL_SWSURFACE, (int)w, (int)h, 32, Rmask, Gmask, Bmask, Amask);
    if (surface) {
        // Sets up a context to be drawn to with surface->pixels as the area to be drawn to
        bitmap_context = CGBitmapContextCreate(
                                               surface->pixels,
                                               surface->w,
                                               surface->h,
                                               bits_per_component,
                                               surface->pitch,
                                               color_space,
                                               bitmap_info
                                               );

        // Draws the image into the context's image_data
        CGContextDrawImage(bitmap_context, rect, image_ref);

        CGContextRelease(bitmap_context);

        // FIXME: Reverse the premultiplied alpha
        if ((bitmap_info & kCGBitmapAlphaInfoMask) == kCGImageAlphaPremultipliedFirst) {
            int i, j;
            Uint8 *p = (Uint8 *)surface->pixels;
            for (i = surface->h * surface->pitch/4; i--; ) {
                #if __LITTLE_ENDIAN__
                    Uint8 A = p[3];
                    if (A) {
                        for (j = 0; j < 3; ++j) {
                            p[j] = (p[j] * 255) / A;
                        }
                    }
                #else
                    Uint8 A = p[0];
                    if (A) {
                        for (j = 1; j < 4; ++j) {
                            p[j] = (p[j] * 255) / A;
                        }
                    }
                #endif /* ENDIAN */
                p += 4;
            }
        }
    }

    if (color_space) {
        CGColorSpaceRelease(color_space);
    }

    return surface;
}
static SDL_Surface* Create_SDL_Surface_From_CGImage_Index(CGImageRef image_ref) {
    size_t w = CGImageGetWidth(image_ref);
    size_t h = CGImageGetHeight(image_ref);
    size_t bits_per_pixel = CGImageGetBitsPerPixel(image_ref);
    size_t bytes_per_row = CGImageGetBytesPerRow(image_ref);

    SDL_Surface* surface;
    SDL_Palette* palette;
    CGColorSpaceRef color_space = CGImageGetColorSpace(image_ref);
    CGColorSpaceRef base_color_space = CGColorSpaceGetBaseColorSpace(color_space);
    size_t num_components = CGColorSpaceGetNumberOfComponents(base_color_space);
    size_t num_entries = CGColorSpaceGetColorTableCount(color_space);
    uint8_t *entry, entries[num_components * num_entries];

    /* What do we do if it's not RGB? */
    if (num_components != 3) {
        SDL_SetError("Unknown colorspace components %lu", num_components);
        return NULL;
    }
    if (bits_per_pixel != 8) {
        SDL_SetError("Unknown bits_per_pixel %lu", bits_per_pixel);
        return NULL;
    }

    CGColorSpaceGetColorTable(color_space, entries);
    surface = SDL_CreateRGBSurface(SDL_SWSURFACE, (int)w, (int)h, bits_per_pixel, 0, 0, 0, 0);
    if (surface) {
        uint8_t* pixels = (uint8_t*)surface->pixels;
        CGDataProviderRef provider = CGImageGetDataProvider(image_ref);
        NSData* data = (id)CGDataProviderCopyData(provider);
        [data autorelease];
        const uint8_t* bytes = (const uint8_t*)[data bytes];
        size_t i;

        palette = surface->format->palette;
        for (i = 0, entry = entries; i < num_entries; ++i) {
            palette->colors[i].r = entry[0];
            palette->colors[i].g = entry[1];
            palette->colors[i].b = entry[2];
            entry += num_components;
        }

        for (i = 0; i < h; ++i) {
            SDL_memcpy(pixels, bytes, w);
            pixels += surface->pitch;
            bytes += bytes_per_row;
        }
    }
    return surface;
}
static SDL_Surface* Create_SDL_Surface_From_CGImage(CGImageRef image_ref) {
    CGColorSpaceRef color_space = CGImageGetColorSpace(image_ref);
    if (CGColorSpaceGetModel(color_space) == kCGColorSpaceModelIndexed) {
        return Create_SDL_Surface_From_CGImage_Index(image_ref);
    } else {
        return Create_SDL_Surface_From_CGImage_RGB(image_ref);
    }
}

static int Internal_isType_UIImage(Stream* rw_ops, CFStringRef uti_string_to_test) {
    int is_type = 0;

    #if defined(ALLOW_UIIMAGE_FALLBACK) && ((TARGET_OS_IPHONE == 1) || (TARGET_IPHONE_SIMULATOR == 1))
        Sint64 start = rw_ops->Position();
        if ((CFStringCompare(uti_string_to_test, kUTTypeICO, 0) == 0) ||
            (CFStringCompare(uti_string_to_test, CFSTR("com.microsoft.cur"), 0) == 0)) {

            // The Win32 ICO file header (14 bytes)
            Uint16 bfReserved;
            Uint16 bfType;
            Uint16 bfCount;
            int type = (CFStringCompare(uti_string_to_test, kUTTypeICO, 0) == 0) ? 1 : 2;

            bfReserved = rw_ops->ReadUInt16();
            bfType = rw_ops->ReadUInt16();
            bfCount = rw_ops->ReadUInt16();
            if ((bfReserved == 0) && (bfType == type) && (bfCount != 0))
                is_type = 1;
        }
        else if (CFStringCompare(uti_string_to_test, kUTTypeBMP, 0) == 0) {
            char magic[2];

            if (rw_ops->ReadBytes(magic, sizeof(magic))) {
                if (strncmp(magic, "BM", 2) == 0) {
                    is_type = 1;
                }
            }
        }
        else if (0 == CFStringCompare(uti_string_to_test, kUTTypeGIF, 0)) {
            char magic[6];

            if (rw_ops->ReadBytes(magic, sizeof(magic))) {
                if ((strncmp(magic, "GIF", 3) == 0) &&
                    ((memcmp(magic + 3, "87a", 3) == 0) ||
                     (memcmp(magic + 3, "89a", 3) == 0)) ) {
                        is_type = 1;
                }
            }
        }
        else if (0 == CFStringCompare(uti_string_to_test, kUTTypeJPEG, 0)) {
            int in_scan = 0;
            Uint8 magic[4];

            // This detection code is by Steaphan Greene <stea@cs.binghamton.edu>
            // Blame me, not Sam, if this doesn't work right. */
            // And don't forget to report the problem to the the sdl list too! */

            if (rw_ops->ReadBytes(magic, 2)) {
                if ( (magic[0] == 0xFF) && (magic[1] == 0xD8) ) {
                    is_type = 1;
                    while (is_type == 1) {
                        if (rw_ops->ReadBytes(magic, 2) != 2) {
                            is_type = 0;
                        } else if( (magic[0] != 0xFF) && (in_scan == 0) ) {
                            is_type = 0;
                        } else if( (magic[0] != 0xFF) || (magic[1] == 0xFF) ) {
                            /* Extra padding in JPEG (legal) */
                            /* or this is data and we are scanning */
                            rw_ops->Skip(-1);
                        } else if (magic[1] == 0xD9) {
                            /* Got to end of good JPEG */
                            break;
                        } else if ( (in_scan == 1) && (magic[1] == 0x00) ) {
                            /* This is an encoded 0xFF within the data */
                        } else if ( (magic[1] >= 0xD0) && (magic[1] < 0xD9) ) {
                            /* These have nothing else */
                        } else if (rw_ops->ReadBytes(magic + 2, 2) != 2) {
                            is_type = 0;
                        } else {
                            /* Yes, it's big-endian */
                            Uint32 start;
                            Uint32 size;
                            Uint32 end;
                            start = rw_ops->Position();
                            size = (magic[2] << 8) + magic[3];
                            end = rw_ops->Skip(size-2);
                            if ( end != start + size - 2 ) is_type = 0;
                            if ( magic[1] == 0xDA ) {
                                /* Now comes the actual JPEG meat */
                                #ifdef  FAST_IS_JPEG
                                    /* Ok, I'm convinced.  It is a JPEG. */
                                    break;
                                #else
                                    /* I'm not convinced.  Prove it! */
                                    in_scan = 1;
                                #endif
                            }
                        }
                    }
                }
            }
        }
        else if (0 == CFStringCompare(uti_string_to_test, kUTTypePNG, 0)) {
            Uint8 magic[4];

            if ( rw_ops->ReadBytes(magic, sizeof(magic)) == sizeof(magic) ) {
                if ( magic[0] == 0x89 &&
                    magic[1] == 'P' &&
                    magic[2] == 'N' &&
                    magic[3] == 'G' ) {
                    is_type = 1;
                }
            }
        }
        else if (0 == CFStringCompare(uti_string_to_test, CFSTR("com.truevision.tga-image"), 0)) {
            //TODO: fill me!
        }
        else if (0 == CFStringCompare(uti_string_to_test, kUTTypeTIFF, 0)) {
            Uint8 magic[4];

            if ( rw_ops->ReadBytes(magic, sizeof(magic)) == sizeof(magic) ) {
                if ( (magic[0] == 'I' &&
                      magic[1] == 'I' &&
                      magic[2] == 0x2a &&
                      magic[3] == 0x00) ||
                    (magic[0] == 'M' &&
                     magic[1] == 'M' &&
                     magic[2] == 0x00 &&
                     magic[3] == 0x2a) ) {
                        is_type = 1;
                    }
            }
        }

        // reset the file pointer
        rw_ops->Seek(start);

    #endif  /* #if defined(ALLOW_UIIMAGE_FALLBACK) && ((TARGET_OS_IPHONE == 1) || (TARGET_IPHONE_SIMULATOR == 1)) */
    return is_type;
}
static int Internal_isType_ImageIO(Stream* rw_ops, CFStringRef uti_string_to_test) {
    int is_type = 0;

    Sint64 start = ((Stream*)rw_ops)->Position();
    CFDictionaryRef hint_dictionary = Create_HintDictionary(uti_string_to_test);
    CGImageSourceRef image_source = Create_CGImageSource_FromStream(rw_ops, hint_dictionary);

    if (hint_dictionary != NULL) {
        CFRelease(hint_dictionary);
    }

    if (image_source == NULL) {
        // reset the file pointer
        ((Stream*)rw_ops)->Seek(start);
        return 0;
    }

    // This will get the UTI of the container, not the image itself.
    // Under most cases, this won't be a problem.
    // But if a person passes an icon file which contains a bmp,
    // the format will be of the icon file.
    // But I think the main SDL_image codebase has this same problem so I'm not going to worry about it.
    CFStringRef uti_type = CGImageSourceGetType(image_source);
    //  CFShow(uti_type);

    // Unsure if we really want conformance or equality
    is_type = (int)UTTypeConformsTo(uti_string_to_test, uti_type);

    CFRelease(image_source);

    // reset the file pointer
	((Stream*)rw_ops)->Seek(start);
    return is_type;
}
static int Internal_isType(Stream* rw_ops, CFStringRef uti_string_to_test) {
    if (rw_ops == NULL)
        return 0;

    #if (TARGET_OS_IPHONE == 1) || (TARGET_IPHONE_SIMULATOR == 1)
        if (USE_UIIMAGE_BACKEND())
            return Internal_isType_UIImage(rw_ops, uti_string_to_test);
        else
    #endif
    return Internal_isType_ImageIO(rw_ops, uti_string_to_test);
}

static SDL_Surface* LoadImageFromRWops_UIImage(Stream* rw_ops, CFStringRef uti_string_hint) {
    SDL_Surface *sdl_surface = NULL;

    #if defined(ALLOW_UIIMAGE_FALLBACK) && ((TARGET_OS_IPHONE == 1) || (TARGET_IPHONE_SIMULATOR == 1))
        NSAutoreleasePool* autorelease_pool = [[NSAutoreleasePool alloc] init];
        UIImage *ui_image;
        int bytes_read = 0;
        // I don't know what a good size is.
        // Max recommended texture size is 1024x1024 on iPhone so maybe base it on that?
        const int block_size = 1024*4;
        char temp_buffer[block_size];

        NSMutableData* ns_data = [[NSMutableData alloc] initWithCapacity:1024*1024*4];
        do {
            bytes_read = rw_ops->ReadBytes(temp_buffer, block_size);
            [ns_data appendBytes:temp_buffer length:bytes_read];
        } while (bytes_read > 0);

        ui_image = [[UIImage alloc] initWithData:ns_data];
        if (ui_image != nil)
            sdl_surface = Create_SDL_Surface_From_CGImage([ui_image CGImage]);
        [ui_image release];
        [ns_data release];
        [autorelease_pool drain];

    #endif  /* #if defined(ALLOW_UIIMAGE_FALLBACK) && ((TARGET_OS_IPHONE == 1) || (TARGET_IPHONE_SIMULATOR == 1)) */
    return sdl_surface;
}
static SDL_Surface* LoadImageFromRWops_ImageIO(Stream* rw_ops, CFStringRef uti_string_hint) {
    CFDictionaryRef hint_dictionary = Create_HintDictionary(uti_string_hint);
    CGImageSourceRef image_source = Create_CGImageSource_FromStream(rw_ops, hint_dictionary);

    if (hint_dictionary != NULL)
        CFRelease(hint_dictionary);
    if (image_source == NULL)
        return NULL;

    CGImageRef image_ref = Create_CGImage_FromCGImageSource(image_source);
    CFRelease(image_source);

    if (image_ref == NULL)
        return NULL;

    SDL_Surface *sdl_surface = Create_SDL_Surface_From_CGImage(image_ref);
    CFRelease(image_ref);

    return sdl_surface;
}
static SDL_Surface* LoadImageFromRWops(Stream* rw_ops, CFStringRef uti_string_hint) {
    #if (TARGET_OS_IPHONE == 1) || (TARGET_IPHONE_SIMULATOR == 1)
        if (USE_UIIMAGE_BACKEND())
            return LoadImageFromRWops_UIImage(rw_ops, uti_string_hint);
        else
    #endif
    return LoadImageFromRWops_ImageIO(rw_ops, uti_string_hint);
}
static SDL_Surface* LoadImageFromFile_UIImage(const char *file) {
    SDL_Surface *sdl_surface = NULL;

    #if defined(ALLOW_UIIMAGE_FALLBACK) && ((TARGET_OS_IPHONE == 1) || (TARGET_IPHONE_SIMULATOR == 1))
        NSAutoreleasePool* autorelease_pool = [[NSAutoreleasePool alloc] init];
        NSString *ns_string = [[NSString alloc] initWithUTF8String:file];
        UIImage *ui_image = [[UIImage alloc] initWithContentsOfFile:ns_string];
        if (ui_image != nil)
            sdl_surface = Create_SDL_Surface_From_CGImage([ui_image CGImage]);
        [ui_image release];
        [ns_string release];
        [autorelease_pool drain];

    #endif  /* #if defined(ALLOW_UIIMAGE_FALLBACK) && ((TARGET_OS_IPHONE == 1) || (TARGET_IPHONE_SIMULATOR == 1)) */
    return sdl_surface;
}
static SDL_Surface* LoadImageFromFile_ImageIO(const char *file) {
    CGImageSourceRef image_source = NULL;

    image_source = Create_CGImageSource_FromFile(file);

    if(NULL == image_source)
        return NULL;

    CGImageRef image_ref = Create_CGImage_FromCGImageSource(image_source);
    CFRelease(image_source);

    if (image_ref == NULL)
        return NULL;
    SDL_Surface *sdl_surface = Create_SDL_Surface_From_CGImage(image_ref);
    CFRelease(image_ref);
    return sdl_surface;
}
static SDL_Surface* LoadImageFromFile(const char *file) {
    #if (TARGET_OS_IPHONE == 1) || (TARGET_IPHONE_SIMULATOR == 1)
        if (USE_UIIMAGE_BACKEND())
            return LoadImageFromFile_UIImage(file);
        else
    #endif
        return LoadImageFromFile_ImageIO(file);
}

/*
#ifdef BMP_USES_IMAGEIO
SDL_Surface* IMG_LoadCUR_RW (Stream* src) {
    // FIXME: Is this a supported type?
    return LoadImageFromRWops(src, CFSTR("com.microsoft.cur"));
}
SDL_Surface* IMG_LoadICO_RW (Stream* src) {
    return LoadImageFromRWops(src, kUTTypeICO);
}
SDL_Surface* IMG_LoadBMP_RW (Stream* src) {
    return LoadImageFromRWops(src, kUTTypeBMP);
}
#endif
// BMP_USES_IMAGEIO

SDL_Surface* IMG_LoadGIF_RW (Stream* src) {
    return LoadImageFromRWops (src, kUTTypeGIF);
}
SDL_Surface* IMG_LoadJPG_RW (Stream* src) {
    return LoadImageFromRWops (src, kUTTypeJPEG);
}
SDL_Surface* IMG_LoadPNG_RW (Stream* src) {
    return LoadImageFromRWops (src, kUTTypePNG);
}
SDL_Surface* IMG_LoadTGA_RW (Stream* src) {
    return LoadImageFromRWops(src, CFSTR("com.truevision.tga-image"));
}
SDL_Surface* IMG_LoadTIF_RW (Stream* src) {
    return LoadImageFromRWops(src, kUTTypeTIFF);
}
// */

// Since UIImage doesn't really support streams well, we should optimize for the file case.
// Apple provides both stream and file loading functions in ImageIO.
// Potentially, Apple can optimize for either case.
SDL_Surface* IMG_Load(const char *file) {
    SDL_Surface* sdl_surface = NULL;

    sdl_surface = LoadImageFromFile(file);
    // if (sdl_surface == NULL) {
    //     // Either the file doesn't exist or ImageIO doesn't understand the format.
    //     // For the latter case, fallback to the native SDL_image handlers.
    //     Stream* src = SDL_RWFromFile(file, "rb");
    //     char *ext = strrchr(file, '.');
    //     if (ext) {
    //         ext++;
    //     }
    //     if (!src) {
    //         /* The error message has been set in SDL_RWFromFile */
    //         return NULL;
    //     }
    //     sdl_surface = IMG_LoadTyped_RW(src, 1, ext);
    // }
    return sdl_surface;
}
