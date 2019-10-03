#ifndef OBJEXPORTER_H
#define OBJEXPORTER_H

#include <string>

namespace Ogre
{
    class Mesh;
    class Texture;
}


class ObjExporter
{
    // intermediate data containers during the conversion
    struct OgreDataVertex
    {
        float position[3];
        float normal[3];
        float texcoord[2];
    };

    struct OgreDataFace
    {
        int index[3];
    };

    struct OgreDataSubMesh
    {
        std::string meshName;
        std::string material;
        std::vector<OgreDataVertex> vertices;
        std::vector<OgreDataFace> faces;
    };

public:
    ObjExporter();

    bool writeToFile(Ogre::Mesh* srcMesh, const QString& sOutFile);

private:
    bool convertToOgreData(const QString& sXmlFile);
    void clearOgreDataSubMesh(OgreDataSubMesh&);
    void clearOgreDataVertex(OgreDataVertex&);
    bool writeObjFile(const QString& sOutObjFile, const QString& sMtlFileName);
    bool writeMtlFile(Ogre::Mesh*, const QString& sOutFile);
    void writeTexture(Ogre::TextureGpu* tex, const std::string& sTexFileName);
    void normalize(OgreDataVertex&);
    std::vector<OgreDataSubMesh> mSubmeshes;
};

#endif // OBJEXPORTER_H