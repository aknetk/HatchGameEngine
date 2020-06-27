#ifndef ENGINE_SCENE_VIEW_H
#define ENGINE_SCENE_VIEW_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED

class Texture;
class Matrix4x4;
class Matrix4x4;

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
    float      FOV = 45.0f;
    float      NearPlane = 0.1f;
    float      FarPlane = 1000.f;
    bool       UsePerspective = false;
    bool       UseDrawTarget = false;
    Texture*   DrawTarget = NULL;
    Matrix4x4* ProjectionMatrix = NULL;
    Matrix4x4* BaseProjectionMatrix = NULL;

};

#endif /* ENGINE_SCENE_VIEW_H */
