#if INTERFACE
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
};
#endif

#include <Engine/ResourceTypes/IModel.h>

#include <Engine/IO/MemoryStream.h>
#include <Engine/IO/ResourceStream.h>

PUBLIC IModel::IModel() {

}

PUBLIC IModel::IModel(const char* filename) {
    ResourceStream* resourceStream = ResourceStream::New(filename);
    if (!resourceStream) return;

    int f1, f2, f3, f4;
    int HasVertColors, PolyType, VertexCount, FrameCount;

    MemoryStream* stream = MemoryStream::New(resourceStream);
    if (!stream) goto CLOSE;

    if (stream->ReadUInt32BE() != 0x4D444C00) { // MDL0
        goto CLOSE;
    }

    HasVertColors = stream->ReadByte();
    PolyType = stream->ReadByte();
    VertexCount = stream->ReadUInt16();
    FrameCount = stream->ReadUInt16();

    if (HasVertColors == 5) {
        for (int i = 0; i < VertexCount; i++) {
            Colors.push_back(stream->ReadByte() << 16 | stream->ReadByte() << 8 | stream->ReadByte());
            stream->ReadByte(); // read A (alpha), we don't it, but we need to read it
        }
    }

    FaceCount = stream->ReadUInt16() / PolyType;
    for (int i = 0; i < FaceCount; i++) {
        f1 = stream->ReadUInt16();
        f2 = stream->ReadUInt16();
        f3 = stream->ReadUInt16();
        if (PolyType == 4) {
            f4 = stream->ReadUInt16();
            Faces.push_back(IFace(f1, f2, f3, f4));
        }
        else
            Faces.push_back(IFace(f1, f2, f3));
    }

    for (int f = 0; f < FrameCount; f++) {
        Vertices.push_back(vector<IVertex>());
        Normals.push_back(vector<IVertex>());
        for (int i = 0; i < VertexCount; i++) {
            Vertices[f].push_back(IVertex(stream->ReadFloat(), stream->ReadFloat(), stream->ReadFloat()));
            Normals[f].push_back(IVertex(stream->ReadFloat(), stream->ReadFloat(), stream->ReadFloat()));
        }
    }

    if (FrameCount == 0) goto CLOSE;

    CLOSE:
        if (stream) stream->Close();
        if (resourceStream) resourceStream->Close();

    // BufferID_V = Graphics::MakeVertexBuffer(this, true);
    // BufferID_N = Graphics::MakeVertexBuffer(this, false);
}

PUBLIC bool IModel::HasColors() {
    return Colors.size() > 0;
}

PUBLIC void IModel::Cleanup() {
    // Does nothing
    // Graphics::DeleteBufferID(BufferID_V);
    // Graphics::DeleteBufferID(BufferID_N);
}
