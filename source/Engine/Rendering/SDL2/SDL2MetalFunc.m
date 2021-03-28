#ifdef IOS

#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "SDL.h"
#include "SDL2MetalFunc.h"

int SDL2MetalFunc_GetMetalSize(int* width, int* height, SDL_Renderer* renderer) {
    @autoreleasepool {
        const CAMetalLayer* swapchain = (__bridge CAMetalLayer *)SDL_RenderGetMetalLayer(renderer);
        *width = swapchain.drawableSize.width;
        *height = swapchain.drawableSize.height;
        return 1;
    }
    return 0;
}

#endif
