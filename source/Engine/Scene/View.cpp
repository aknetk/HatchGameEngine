#if INTERFACE
#include <Engine/Includes/Standard.h>
#include <Engine/Math/Matrix4x4.h>

class View {
public:
    bool       Active = false;
    bool       Visible = true;
    bool       Software = false;
    float      X = 0.0f;
    float      Y = 0.0f;
    float      Z = 0.0f;
    float      RotateX = 0.0f;
    float      RotateY = 0.0f;
    float      RotateZ = 0.0f;
    float      Width = 1.0f;
    float      Height = 1.0f;
    float      WindowX = 0.0f;
    float      WindowY = 0.0f;
    float      WindowWidth = 1.0f;
    float      WindowHeight = 1.0f;
    int        Stride = 1;
    float      FOV = 45.0f;
    float      NearPlane = 0.1f;
    float      FarPlane = 1000.f;
    bool       UsePerspective = false;
    bool       UseDrawTarget = false;
    Texture*   DrawTarget = NULL;
    Matrix4x4* ProjectionMatrix = NULL;
    Matrix4x4* BaseProjectionMatrix = NULL;
};
#endif

// #include <Engine/Scene/View.h>
// #include <Engine/Diagnostics/Memory.h>
