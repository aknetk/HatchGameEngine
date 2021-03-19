/*

Implemented by Jimita
https://github.com/Jimita

*/

#ifndef COLLADAMODEL_H
#define COLLADAMODEL_H

#include <Engine/Includes/Standard.h>
#include <Engine/Rendering/3D.h>
#include <Engine/ResourceTypes/IModel.h>

typedef enum {
    DAE_X_UP,
    DAE_Y_UP,
    DAE_Z_UP,
} ColladaAssetAxis;

typedef enum {
    DAE_NODE_REGULAR,
    DAE_NODE_JOINT,
} ColladaNodeType;

//
// Meshes
//

struct ColladaMeshFloatArray {
    char *Id;
    int   Count;

    vector<float> Contents;
};

struct ColladaMeshAccessor {
    ColladaMeshFloatArray *Source;

    int Count;
    int Stride;
};

struct ColladaMeshSource {
    char *Id;

    vector<ColladaMeshFloatArray*> FloatArrays;
    vector<ColladaMeshAccessor*>   Accessors;
};

struct ColladaInput {
    char *Semantic;
    int   Offset;

    ColladaMeshSource    *Source;
    vector<ColladaInput*> Children;
};

//
// Textures
//

struct ColladaImage {
    char *Id;
    char *Path;
};

struct ColladaSurface {
    char *Id;
    char *Type;

    ColladaImage *Image;
};

struct ColladaSampler {
    char *Id;

    ColladaSurface *Surface;

    int wrap_s,    wrap_t;
    int minfilter, magfilter, mipfilter;
};

struct ColladaPhongComponent {
    float Color[4];

    ColladaSampler *Sampler;
};

struct ColladaEffect {
    char *Id;

    ColladaPhongComponent Specular;
    ColladaPhongComponent Ambient;
    ColladaPhongComponent Emission;
    ColladaPhongComponent Diffuse;

    float Shininess;
    float Transparency;
    float IndexOfRefraction;
};

struct ColladaMaterial {
    char *Id;
    char *Name;
    char *EffectLink;

    ColladaEffect *Effect;
};

//
// Model data
//

struct ColladaTriangles {
    int  Count;
    char *Id;

    ColladaMaterial       Material;
    vector<ColladaInput*> Inputs;
    vector<int>           Primitives;
};

struct ColladaVertices {
    char *Id;

    vector<ColladaInput*> Inputs;
};

struct ColladaMesh {
    vector<ColladaMeshSource*> SourceList;

    ColladaVertices   Vertices;
    ColladaTriangles  Triangles;
    ColladaMaterial  *Material;
};

struct ColladaGeometry {
    char *Id;
    char *Name;

    ColladaMesh *Mesh;
};

struct ColladaNode {
    ColladaNodeType  Type;
    ColladaNode     *Node;

    char *Id;
    char *Name;

    float            Matrix[16];
    ColladaMesh*     Mesh;
    ColladaMaterial* Material;
};

struct ColladaScene {
    vector<ColladaNode*>     Nodes;
};

struct ColladaModel {
    vector<ColladaMaterial*> Materials;
    vector<ColladaSurface*>  Surfaces;
    vector<ColladaSampler*>  Samplers;
    vector<ColladaEffect*>   Effects;
    vector<ColladaImage*>    Images;

    vector<ColladaGeometry*> Geometries;
    vector<ColladaMesh*>     Meshes;

    vector<ColladaNode*>     Nodes;
    vector<ColladaScene*>    Scenes;

    ColladaAssetAxis         Axis;
};

#endif
