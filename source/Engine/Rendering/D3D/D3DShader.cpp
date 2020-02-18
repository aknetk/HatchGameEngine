#if INTERFACE
#include <Engine/Includes/Standard.h>
#include <Engine/Includes/StandardSDL2.h>

#include <Engine/IO/Stream.h>

class D3DShader {
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
};
#endif

#include <Engine/Rendering/D3D/D3DShader.h>

#include <Engine/Diagnostics/Log.h>

#define CHECK_D3D() \
    D3DShader::CheckD3DError(__LINE__)

PUBLIC        D3DShader::D3DShader(const char** vertexShaderSource, size_t vsSZ, const char** fragmentShaderSource, size_t fsSZ) {
    GLint compiled = GL_FALSE;
    ProgramID = glCreateProgram(); CHECK_D3D();

    VertexProgramID = glCreateShader(GL_VERTEX_SHADER); CHECK_D3D();
    glShaderSource(VertexProgramID, vsSZ / sizeof(GLchar*), vertexShaderSource, NULL); CHECK_D3D();
    glCompileShader(VertexProgramID); CHECK_D3D();
    glGetShaderiv(VertexProgramID, GL_COMPILE_STATUS, &compiled); CHECK_D3D();
    if (compiled != GL_TRUE) {
        Log::Print(Log::LOG_ERROR, "Unable to compile vertex shader %d!", VertexProgramID);
        CheckShaderError(VertexProgramID);
        CheckD3DError(__LINE__);
        return;
    }

    FragmentProgramID = glCreateShader(GL_FRAGMENT_SHADER); CHECK_D3D();
    glShaderSource(FragmentProgramID, fsSZ / sizeof(GLchar*), fragmentShaderSource, NULL); CHECK_D3D();
    glCompileShader(FragmentProgramID); CHECK_D3D();
    glGetShaderiv(FragmentProgramID, GL_COMPILE_STATUS, &compiled); CHECK_D3D();
    if (compiled != GL_TRUE) {
        Log::Print(Log::LOG_ERROR, "Unable to compile fragment shader %d!", FragmentProgramID);
        CheckShaderError(FragmentProgramID);
        CheckD3DError(__LINE__);
        return;
    }

    AttachAndLink();
}
PUBLIC        D3DShader::D3DShader(Stream* streamVS, Stream* streamFS) {
    GLint compiled = GL_FALSE;
    ProgramID = glCreateProgram(); CHECK_D3D();

    size_t lenVS = streamVS->Length();
    size_t lenFS = streamFS->Length();

    char* sourceVS = (char*)malloc(lenVS);
    char* sourceFS = (char*)malloc(lenFS);

    // const char* sourceVSMod[2];
    const char* sourceFSMod[2];

    streamVS->ReadBytes(sourceVS, lenVS);
    sourceVS[lenVS] = 0;

    streamFS->ReadBytes(sourceFS, lenFS);
    sourceFS[lenFS] = 0;

    #if GL_ES_VERSION_2_0 || GL_ES_VERSION_3_0
        sourceFSMod[0] = "precision mediump float;\n";
    #else
        sourceFSMod[0] = "\n";
    #endif
    sourceFSMod[1] = sourceFS;

    VertexProgramID = glCreateShader(GL_VERTEX_SHADER); CHECK_D3D();

    glShaderSource(VertexProgramID, 1, &sourceVS, NULL); CHECK_D3D();
    glCompileShader(VertexProgramID); CHECK_D3D();
    glGetShaderiv(VertexProgramID, GL_COMPILE_STATUS, &compiled); CHECK_D3D();
    if (compiled != GL_TRUE) {
        Log::Print(Log::LOG_ERROR, "Unable to compile vertex shader %d!", VertexProgramID);
        CheckShaderError(VertexProgramID);
        CheckD3DError(__LINE__);

        free(sourceVS);
        free(sourceFS);
        return;
    }

    FragmentProgramID = glCreateShader(GL_FRAGMENT_SHADER); CHECK_D3D();
    glShaderSource(FragmentProgramID, 2, sourceFSMod, NULL); CHECK_D3D();
    glCompileShader(FragmentProgramID); CHECK_D3D();
    glGetShaderiv(FragmentProgramID, GL_COMPILE_STATUS, &compiled); CHECK_D3D();
    if (compiled != GL_TRUE) {
        Log::Print(Log::LOG_ERROR, "Unable to compile fragment shader %d!", FragmentProgramID);
        CheckShaderError(FragmentProgramID);
        CheckD3DError(__LINE__);

        free(sourceVS);
        free(sourceFS);
        return;
    }

    free(sourceVS);
    free(sourceFS);

    AttachAndLink();
}
PRIVATE void  D3DShader::AttachAndLink() {
    glAttachShader(ProgramID, VertexProgramID);                 CHECK_D3D();
    glAttachShader(ProgramID, FragmentProgramID);               CHECK_D3D();

    glBindAttribLocation(ProgramID, 0, "i_position");           CHECK_D3D();
    glBindAttribLocation(ProgramID, 1, "i_uv");                 CHECK_D3D();

    glLinkProgram(ProgramID);                                   CHECK_D3D();

    GLint isLinked = GL_FALSE;
    glGetProgramiv(ProgramID, GL_LINK_STATUS, (int*)&isLinked); CHECK_D3D();
    if (isLinked != GL_TRUE) {
        CheckProgramError(ProgramID);
    }

    LocProjectionMatrix = GetUniformLocation("u_projectionMatrix");
    LocModelViewMatrix = GetUniformLocation("u_modelViewMatrix");

    LocPosition = GetAttribLocation("i_position");
    LocTexCoord = GetAttribLocation("i_uv");
    LocTexture = GetUniformLocation("u_texture");
    LocTextureU = GetUniformLocation("u_textureU");
    LocTextureV = GetUniformLocation("u_textureV");
    LocColor = GetUniformLocation("u_color");
}

PUBLIC bool   D3DShader::CheckShaderError(GLuint shader) {
    int infoLogLength = 0;
    int maxLength = infoLogLength;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

    char* infoLog = new char[maxLength];
    glGetShaderInfoLog(shader, maxLength, &infoLogLength, infoLog);
    infoLog[strlen(infoLog) - 1] = 0;

    if (infoLogLength > 0)
        Log::Print(Log::LOG_ERROR, "%s", infoLog + 7);

    delete[] infoLog;
    return false;
}
PUBLIC bool   D3DShader::CheckProgramError(GLuint prog) {
    int infoLogLength = 0;
    int maxLength = infoLogLength;

    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &maxLength);

    char* infoLog = new char[maxLength];
    glGetProgramInfoLog(prog, maxLength, &infoLogLength, infoLog);
    infoLog[strlen(infoLog) - 1] = 0;

    if (infoLogLength > 0)
        Log::Print(Log::LOG_ERROR, "%s", infoLog + 7);

    delete[] infoLog;
    return false;
}

PUBLIC GLuint D3DShader::Use() {
    glUseProgram(ProgramID); CheckD3DError(__LINE__);

    // glEnableVertexAttribArray(1);

    CheckD3DError(__LINE__);
    return ProgramID;
}

PUBLIC GLint  D3DShader::GetAttribLocation(const char* identifier) {
    GLint value = glGetAttribLocation(ProgramID, identifier);
    CheckD3DError(__LINE__);
    return value;
}
PUBLIC GLint  D3DShader::GetUniformLocation(const char* identifier) {
    GLint value = glGetUniformLocation(ProgramID, identifier);
    CheckD3DError(__LINE__);
    return value;
}

PUBLIC void   D3DShader::Dispose() {
    glDeleteProgram(ProgramID);
}

PUBLIC STATIC bool D3DShader::CheckD3DError(int line) {
    int error = 0;
    const char* errstr = "";
    if (error) {
        Log::Print(Log::LOG_ERROR, "Direct3D error on line %d: %s", line, errstr);
        return true;
    }
    return false;
}

#undef CHECK_D3D
