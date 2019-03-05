#ifndef OBJIMPORTER_H
#define OBJIMPORTER_H

#include <string>
#include <array>
#include "tiny_obj_loader.h"

class QXmlStreamWriter;
class OgreManager;


struct UniqueVertex
{
    int v = 0;
    int n = 0;
    int t = 0;

    UniqueVertex(const tinyobj::index_t& idx)
    {
        v = idx.vertex_index;
        n = idx.normal_index;
        t = idx.texcoord_index;
    }
};


class ObjImporter
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

        bool bNeedGenerateNormals = false;
    };

public:
    ObjImporter(OgreManager* ogre);

    bool import(const QString& sObjFile, const QString& sOgreMeshFile);
    void setZUpToYUp(bool b) { mZUpToYUp = b; }

private:
    void writeMeshXML(const QString& sOutFile);
    void writeXMLOneSubMesh(QXmlStreamWriter& xout, const OgreDataSubMesh& mesh01);
    void writeXMLFaces(QXmlStreamWriter& xout, const OgreDataSubMesh& mesh01);
    void writeXMLGeometry(QXmlStreamWriter& xout, const OgreDataSubMesh& mesh01);

    void convertToOgreData();
    OgreDataSubMesh convertObjMeshToOgreData(const tinyobj::mesh_t&);
    
    void importOgreMeshFromXML(const QString& sXMLFile, Ogre::v1::MeshPtr& meshV1Ptr);
    void generateNormalVectors(OgreDataSubMesh& submesh);

    void convertFromZUpToYUp(OgreDataSubMesh& submesh);

private:
    // the results of tinyobj loader
    tinyobj::attrib_t mObjAttrib;
    std::vector<tinyobj::shape_t> mTinyObjShapes;
    std::vector<tinyobj::material_t> mTinyObjMaterials;

    // re-hash the obj indexes
    std::vector<UniqueVertex> mUniqueVerticesVec;
    std::map<UniqueVertex, int> mUniqueVerticesIndexMap;

    std::set<std::string> mImportedMaterials;
    std::vector<OgreDataSubMesh> mOgreSubMeshes;

    bool mZUpToYUp = true;

    OgreManager* mOgre = nullptr;
};

#endif // OBJIMPORTER_H