#ifndef OBJIMPORTER_H
#define OBJIMPORTER_H

#include <string>
#include <array>
#include "tiny_obj_loader.h"

class QXmlStreamWriter;
class OgreManager;

namespace Ogre
{
class HlmsPbsDatablock;
}


struct UniqueIndex
{
    int v = 0;
    int n = 0;
    int t = 0;

    UniqueIndex(const tinyobj::index_t& idx)
    {
        v = idx.vertex_index;
        n = idx.normal_index;
        t = idx.texcoord_index;
    }
};

struct OgreDataVertex
{
    float position[3];
    float normal[3];
    float texcoord[2];
};

class ObjImporter
{
    struct OgreDataFace
    {
        int index[3];
    };

    struct OgreDataSubMesh
    {
        std::string meshName;
        std::string material;
        std::vector<OgreDataVertex> vertices;
        std::vector<int32_t> indexes;
        Ogre::Aabb aabb;

        bool bNeedGenerateNormals = false;
    };

public:
    explicit ObjImporter();

    Ogre::MeshPtr import(const QString& sObjFile);
    void setZUpToYUp(bool b) { mZUpToYUp = b; }

private:
    Ogre::MeshPtr createOgreMeshes();
    void convertToOgreData();
    OgreDataVertex getVertex(const tinyobj::index_t&);
    
    void generateNormalVectors(OgreDataSubMesh& submesh);
    void convertFromZUpToYUp(OgreDataSubMesh& submesh);
    void generateAABB(OgreDataSubMesh& submesh);
    Ogre::HlmsPbsDatablock* importMaterial(const tinyobj::material_t& srcMtl);

private:
    // the results of tinyobj loader
    tinyobj::attrib_t mObjAttrib;
    std::vector<tinyobj::shape_t> mTinyObjShapes;
    std::vector<tinyobj::material_t> mTinyObjMaterials;

    std::set<std::string> mImportedMaterials;
    std::vector<OgreDataSubMesh> mOgreSubMeshes;

    bool mZUpToYUp = true;
    QString mFileName;
};

#endif // OBJIMPORTER_H