#ifndef ENGINE_RESOURCETYPES_IMODEL_H
#define ENGINE_RESOURCETYPES_IMODEL_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED


#include <Engine/Includes/Standard.h>
#include <Engine/Rendering/3D.h>
#include <Engine/Graphics.h>
#include <Engine/IO/Stream.h>

class IModel {
public:
    Vector3* PositionBuffer;
    Vector2* UVBuffer;
    Uint32*  ColorBuffer;
    Sint16*  VertexIndexBuffer;
    Uint16   VertexCount;
    Uint16   VertexIndexCount;
    Uint16   FrameCount;
    Uint8    VertexFlag;
    Uint8    FaceVertexCount;

    IModel();
    IModel(const char* filename);
    bool Load(Stream* stream, const char* filename);
    bool ReadRSDK(Stream* stream);
    bool HasColors();
    void Cleanup();
};

#endif /* ENGINE_RESOURCETYPES_IMODEL_H */
