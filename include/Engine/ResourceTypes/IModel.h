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

class IModel {
public:
    vector<uint32_t>        Colors;
    vector<IFace>           Faces;
    vector<vector<IVertex>> Vertices;
    vector<vector<IVertex>> Normals;
    vector<vector<IVertex>> UVs;
    int                     FaceCount;
    unsigned int            BufferID_V;
    unsigned int            BufferID_N;

    IModel();
    IModel(const char* filename);
    bool HasColors();
    void Cleanup();
};

#endif /* ENGINE_RESOURCETYPES_IMODEL_H */
