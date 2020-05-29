#ifndef ENGINE_RENDERING_D3D_D3DSHADER_H
#define ENGINE_RENDERING_D3D_D3DSHADER_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED


#include <Engine/Includes/Standard.h>
#include <Engine/Includes/StandardSDL2.h>
#include <Engine/IO/Stream.h>

class D3DShader {
private:
    void  AttachAndLink();

public:
    Uint32 ProgramID;
    Uint32 VertexProgramID;
    Uint32 FragmentProgramID;
    int    LocProjectionMatrix;
    int    LocModelViewMatrix;
    int    LocPosition;
    int    LocTexCoord;
    int    LocTexture;
    int    LocTextureU;
    int    LocTextureV;
    int    LocColor;
    char   FilenameV[256];
    char   FilenameF[256];

           D3DShader(const char** vertexShaderSource, size_t vsSZ, const char** fragmentShaderSource, size_t fsSZ);
           D3DShader(Stream* streamVS, Stream* streamFS);
    bool   CheckShaderError(GLuint shader);
    bool   CheckProgramError(GLuint prog);
    GLuint Use();
    GLint  GetAttribLocation(const char* identifier);
    GLint  GetUniformLocation(const char* identifier);
    void   Dispose();
    static bool CheckD3DError(int line);
};

#endif /* ENGINE_RENDERING_D3D_D3DSHADER_H */
