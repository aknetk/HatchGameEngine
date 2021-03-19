#ifndef ENGINE_RENDERER_GL_INCLUDES
#define ENGINE_RENDERER_GL_INCLUDES

// OpenGL bindings
#if WIN32
    #include <GL/glew.h>
#elif MACOSX
    #include <OpenGL/gl3.h>
    #include <OpenGL/gl3ext.h>
#elif SWITCH
    #include <GLES2/gl2.h>
    #include <SDL2/SDL_opengl.h>
    #include <GLES2/gl2ext.h>
#elif IOS
    #include <OpenGLES/ES3/gl.h>
    #include <OpenGLES/ES3/glext.h>
#elif ANDROID
    #include <SDL_opengles2.h>
#endif

#endif /* ENGINE_RENDERER_GL_INCLUDES */
