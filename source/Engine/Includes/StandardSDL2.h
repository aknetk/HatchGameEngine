#ifndef STANDARDSDL2_H
#define STANDARDSDL2_H

#if WIN32
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_syswm.h>

    #include <GL/glew.h>
#elif MACOSX
    #include <SDL.h>
    #include <OpenGL/gl3.h>
    #include <OpenGL/gl3ext.h>
#elif SWITCH
    #include <SDL2/SDL.h>

    #include <GLES2/gl2.h>
    #include <SDL2/SDL_opengl.h>
    #include <GLES2/gl2ext.h>

    #include <switch.h>
#elif IOS
    #include <SDL.h>
    #include <OpenGLES/ES3/gl.h>
    #include <OpenGLES/ES3/glext.h>
#elif ANDROID
    #include <SDL.h>
    #include <SDL_opengles2.h>
#endif

typedef uint32_t uint;

#endif // STANDARDSDL2_H
