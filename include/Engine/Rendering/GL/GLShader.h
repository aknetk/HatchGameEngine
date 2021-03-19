#ifndef ENGINE_RENDERING_GL_GLSHADER_H
#define ENGINE_RENDERING_GL_GLSHADER_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED

class Matrix4x4;
class Matrix4x4;

#include <Engine/Includes/Standard.h>
#include <Engine/Rendering/GL/Includes.h>
#include <Engine/IO/Stream.h>

class GLShader {
private:
    void  AttachAndLink();

public:
    GLuint ProgramID;
    GLuint VertexProgramID;
    GLuint FragmentProgramID;
    GLint  LocProjectionMatrix;
    GLint  LocModelViewMatrix;
    GLint  LocPosition;
    GLint  LocTexCoord;
    GLint  LocTexture;
    GLint  LocTextureU;
    GLint  LocTextureV;
    GLint  LocColor;
    char   FilenameV[256];
    char   FilenameF[256];
    // Cache stuff
    float      CachedBlendColors[4];
    Matrix4x4* CachedProjectionMatrix = NULL;
    Matrix4x4* CachedModelViewMatrix = NULL;

           GLShader(const GLchar** vertexShaderSource, size_t vsSZ, const GLchar** fragmentShaderSource, size_t fsSZ);
           GLShader(Stream* streamVS, Stream* streamFS);
    bool   CheckShaderError(GLuint shader);
    bool   CheckProgramError(GLuint prog);
    GLuint Use();
    GLint  GetAttribLocation(const GLchar* identifier);
    GLint  GetUniformLocation(const GLchar* identifier);
    void   Dispose();
    static bool CheckGLError(int line);
};

#endif /* ENGINE_RENDERING_GL_GLSHADER_H */
