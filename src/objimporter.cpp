
#include "stdafx.h"
#include "objimporter.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <QTemporaryDir>
#include <QXmlStreamWriter>
#include <QVector3D>
#include <QProgressDialog>
#include <QApplication>

#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreMesh2Serializer.h"
#include "OgreSubMesh2.h"

#include "OgreHlmsManager.h"
#include "OgreHlmsTextureManager.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsJsonPbs.h"

#include "OgreXML/OgreXMLMeshSerializer.h"

#ifdef PROFILING

#define CLOCK_LINENUM_CAT( name, ln ) name##ln
#define CLOCK_LINENUM( name, ln ) CLOCK_LINENUM_CAT( name, ln )
#define CLOCK_BEGIN CLOCK_LINENUM(t1, __LINE__, begin_section)
#define CLOCK_END   CLOCK_LINENUM(t2, __LINE__, end_section)
#define PROFILE( f ) \
    clock_t CLOCK_BEGIN = clock(); \
    f; \
    clock_t CLOCK_END = clock(); \
    qDebug() << #f << ": Use" << float( CLOCK_END - CLOCK_BEGIN ) / CLOCKS_PER_SEC << "sec";
#else
#define PROFILE( f ) f
#endif


bool operator<(const UniqueIndex& l, const UniqueIndex& r)
{
    return std::tie(l.v, l.n, l.t) < std::tie(r.v, r.n, r.t);
}

bool operator==(const OgreDataVertex& l, const OgreDataVertex& r)
{
    return
        l.position[0] == r.position[0] &&
        l.position[1] == r.position[1] &&
        l.position[2] == r.position[2] &&
        l.normal[0] == r.normal[0] &&
        l.normal[1] == r.normal[1] &&
        l.normal[2] == r.normal[2] &&
        l.texcoord[0] == r.texcoord[0] &&
        l.texcoord[1] == r.texcoord[1];
}

namespace std
{
template<> struct hash<OgreDataVertex>
{
    size_t operator()(OgreDataVertex const& vertex) const
    {
        size_t ret = hash<float>()(vertex.position[0]);
        ret = ret ^ (hash<float>()(vertex.position[1]) << 1);
        ret = ret ^ (hash<float>()(vertex.position[2]) >> 1);
        ret = ret << 1;

        ret = ret ^ (hash<float>()(vertex.normal[0]) << 1);
        ret = ret ^ (hash<float>()(vertex.normal[1]) >> 1);
        ret = ret ^ (hash<float>()(vertex.normal[2]) << 1);
        ret = ret >> 1;

        ret = ret ^ (hash<float>()(vertex.texcoord[0]) << 1);
        ret = ret ^ (hash<float>()(vertex.texcoord[1]) >> 1);

        return ret;
    }
};
}

ObjImporter::ObjImporter(OgreManager* ogre)
{
    mOgre = ogre;
}

Ogre::MeshPtr ObjImporter::import(const QString& sObjFile)
{
    QFileInfo info(sObjFile);
    mFileName = info.fileName();

    QProgressDialog progress(nullptr, Qt::Dialog | Qt::WindowTitleHint);
    progress.setLabelText(QString("Converting %1...").arg(info.fileName()));
    progress.setRange(0, 100);
    progress.setModal(true);
    progress.show();
    QApplication::processEvents();

    std::string sError;
    std::string sMtlBasePath = info.absolutePath().toStdString() + "/";
    PROFILE(bool b = tinyobj::LoadObj(&mObjAttrib, &mTinyObjShapes, &mTinyObjMaterials, &sError, sObjFile.toStdString().c_str(), sMtlBasePath.c_str(), true));
    
    if (!sError.empty())
    {
        qDebug() << "Error:" << sError.c_str();
    }

    progress.setValue(10);
    QApplication::processEvents();

    for (tinyobj::material_t& mtl : mTinyObjMaterials)
    {
        if (mImportedMaterials.count(mtl.name) == 0)
        {
            importMaterial(mtl);
            mImportedMaterials.insert(mtl.name);
        }
    }

    progress.setValue(30);
    QApplication::processEvents();

    convertToOgreData();

    progress.setValue(40);
    QApplication::processEvents();

    Ogre::MeshPtr mesh = createOgreMeshes();

    progress.setValue(80);
    QApplication::processEvents();

    return mesh;
}

OgreDataVertex ObjImporter::getVertex(const tinyobj::index_t& index)
{
    OgreDataVertex v1;
    {
        float posX = mObjAttrib.vertices[index.vertex_index * 3 + 0];
        float posY = mObjAttrib.vertices[index.vertex_index * 3 + 1];
        float posZ = mObjAttrib.vertices[index.vertex_index * 3 + 2];

        v1.position[0] = posX;
        v1.position[1] = posY;
        v1.position[2] = posZ;

        if (index.normal_index != -1)
        {
            float normalX = mObjAttrib.normals[index.normal_index * 3 + 0];
            float normalY = mObjAttrib.normals[index.normal_index * 3 + 1];
            float normalZ = mObjAttrib.normals[index.normal_index * 3 + 2];
            v1.normal[0] = normalX;
            v1.normal[1] = normalY;
            v1.normal[2] = normalZ;
        }
        else
        {
            //v1.bNeedGenerateNormals = true;
        }

        if (index.texcoord_index != -1)
        {
            float texCoordU = mObjAttrib.texcoords[index.texcoord_index * 2 + 0];
            float texCoordV = mObjAttrib.texcoords[index.texcoord_index * 2 + 1];
            v1.texcoord[0] = texCoordU;
            v1.texcoord[1] = 1.0 - texCoordV;
        }
        else
        {
            v1.texcoord[0] = 0;
            v1.texcoord[1] = 0;
        }
    }
    return v1;
}

 Ogre::MeshPtr ObjImporter::createOgreMeshes()
{
    Ogre::Root& root = Ogre::Root::getSingleton();
    Ogre::RenderSystem* renderSystem = root.getRenderSystem();
    Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();

    //Create the mesh
    Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().createManual(
        mFileName.toStdString(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

    //Create one submesh
    for (const OgreDataSubMesh& m : mOgreSubMeshes)
    {
        Ogre::SubMesh* subMesh = mesh->createSubMesh();

        //Vertex declaration
        Ogre::VertexElement2Vec vertexElements
        {
            Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION),
            Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL),
            Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES)
        };

        //For immutable buffers, it is mandatory that cubeVertices is not a null pointer.
        auto vBuffer = reinterpret_cast<OgreDataVertex*>(OGRE_MALLOC_SIMD(sizeof(OgreDataVertex) * m.vertices.size(),
                                                                          Ogre::MEMCATEGORY_GEOMETRY));

        //Fill the data.
        memcpy(vBuffer, m.vertices.data(), sizeof(OgreDataVertex) * m.vertices.size());
        Ogre::VertexBufferPacked* vertexBuffer = 0;
        try
        {
            //Create the actual vertex buffer.
            vertexBuffer = vaoManager->createVertexBuffer(vertexElements, m.vertices.size(),
                                                          Ogre::BT_IMMUTABLE,
                                                          vBuffer, true);
        }
        catch (Ogre::Exception &e)
        {
            OGRE_FREE_SIMD(vertexBuffer, Ogre::MEMCATEGORY_GEOMETRY);
            vertexBuffer = 0;
            //throw e;
        }

        Ogre::VertexBufferPackedVec vertexBuffers{ vertexBuffer };

        auto iBuffer = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(sizeof(Ogre::uint32) * m.indexes.size(),
                                                                        Ogre::MEMCATEGORY_GEOMETRY));
        memcpy(iBuffer, m.indexes.data(), sizeof(Ogre::uint32) * m.indexes.size());

        Ogre::IndexBufferPacked* indexBuffer;
        try
        {
            indexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT,
                                                        m.indexes.size(),
                                                        Ogre::BT_IMMUTABLE,
                                                        iBuffer, true);
        }
        catch (Ogre::Exception &e)
        {
            // When keepAsShadow = true, the memory will be freed when the index buffer is destroyed.
            // However if for some weird reason there is an exception raised, the memory will
            // not be freed, so it is up to us to do so.
            // The reasons for exceptions are very rare. But we're doing this for correctness.
            OGRE_FREE_SIMD(indexBuffer, Ogre::MEMCATEGORY_GEOMETRY);
            indexBuffer = 0;
            //throw e;
        }

        Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vertexBuffers, indexBuffer, Ogre::OT_TRIANGLE_LIST);

        //Each Vao pushed to the vector refers to an LOD level.
        //Must be in sync with mesh->mLodValues & mesh->mNumLods if you use more than one level
        subMesh->mVao[Ogre::VpNormal].push_back(vao);
        //Use the same geometry for shadow casting.
        subMesh->mVao[Ogre::VpShadow].push_back(vao);

        //Set the bounds to get frustum culling and LOD to work correctly.
        mesh->_setBounds(Ogre::Aabb(Ogre::Vector3::ZERO, Ogre::Vector3::UNIT_SCALE), false);
        mesh->_setBoundingSphereRadius(1.732f);
    }
    return mesh;
}

void ObjImporter::convertToOgreData()
{
    mOgreSubMeshes.clear();

    for (int s = 0; s < mTinyObjShapes.size(); ++s)
    {
        tinyobj::mesh_t& mesh01 = mTinyObjShapes[s].mesh;
        qDebug() << "    Converting Mesh=" << mTinyObjShapes[s].name.c_str();
        qDebug() << "      face count=" << mesh01.indices.size() / 3;

        Q_ASSERT(mesh01.indices.size() % 3 == 0);

        mVerticesVec.clear();
        mIndexesVec.clear();

        std::unordered_map<OgreDataVertex, uint32_t> uniqueVertices;

        // build indexes
        for (tinyobj::index_t& i : mesh01.indices)
        {
            OgreDataVertex v = getVertex(i);

            if (uniqueVertices.count(v) == 0)
            {
                uniqueVertices[v] = mVerticesVec.size();
                mVerticesVec.push_back(v);
            }
            mIndexesVec.push_back(uniqueVertices[v]);
        }

        OgreDataSubMesh subMesh;
        subMesh.meshName = mTinyObjShapes[s].name;
        subMesh.vertices = mVerticesVec;
        subMesh.indexes = mIndexesVec;

        //OgreDataSubMesh subMesh = convertObjMeshToOgreData(mesh01);

        /*
        if (subMesh.bNeedGenerateNormals)
        {
            generateNormalVectors(OUT subMesh);
        }

        if (mZUpToYUp)
        {
            convertFromZUpToYUp(subMesh);
        }
        */
        
        if (mesh01.material_ids[0] >= 0)
        {
            subMesh.material = mTinyObjMaterials[mesh01.material_ids[0]].name;
        }
        subMesh.meshName = mTinyObjShapes[s].name;
        mOgreSubMeshes.push_back(subMesh);
        
        QApplication::processEvents();
    }
}


void AssignVector(float f[3], const Ogre::Vector3& v)
{
    f[0] = v.x;
    f[1] = v.y;
    f[2] = v.z;
}

void ObjImporter::generateNormalVectors(OgreDataSubMesh& submesh)
{
    struct NormalSum
    {
        Ogre::Vector3 normal{ 0, 0, 0 };
        int count = 0;
    };

    std::vector<NormalSum> normalSums;
    normalSums.resize(submesh.vertices.size());
    
    /*
    for (OgreDataFace& f : submesh.faces)
    {
        Ogre::Vector3 p1(submesh.vertices[f.index[0]].position);
        Ogre::Vector3 p2(submesh.vertices[f.index[1]].position);
        Ogre::Vector3 p3(submesh.vertices[f.index[2]].position);

        Ogre::Vector3 v1 = p2 - p1;
        Ogre::Vector3 v2 = p3 - p1;

        Ogre::Vector3 theNormal = v2.crossProduct(v1);
        theNormal.normalise();

        //AssignVector(submesh.vertices[f.index[0]].normal, theNormal);
        //AssignVector(submesh.vertices[f.index[1]].normal, theNormal);
        //AssignVector(submesh.vertices[f.index[2]].normal, theNormal);

        normalSums[f.index[0]].normal += theNormal;
        normalSums[f.index[0]].count += 1;

        normalSums[f.index[1]].normal += theNormal;
        normalSums[f.index[1]].count += 1;

        normalSums[f.index[2]].normal += theNormal;
        normalSums[f.index[2]].count += 1;
    }
    */

    for (NormalSum& n : normalSums)
    {
        n.normal = n.normal / float(n.count);
    }

    for (int i = 0; i < submesh.vertices.size(); ++i)
    {
        AssignVector(submesh.vertices[i].normal, normalSums[i].normal);
    }
}

void ObjImporter::convertFromZUpToYUp(OgreDataSubMesh& submesh)
{
    for (OgreDataVertex& v : submesh.vertices)
    {
        std::swap(v.position[1], v.position[2]);
        std::swap(v.normal[1], v.normal[2]);

        v.position[2] = -v.position[2];
        v.normal[2] = -v.normal[2];
    }
}

Ogre::HlmsPbsDatablock* ObjImporter::importMaterial(const tinyobj::material_t& srcMtl)
{
    Ogre::Root& root = Ogre::Root::getSingleton();
    Ogre::HlmsManager* hlmsManager = root.getHlmsManager();
    Ogre::HlmsTextureManager* hlmsTextureManager = hlmsManager->getTextureManager();

    Q_ASSERT(dynamic_cast<Ogre::HlmsPbs*>(hlmsManager->getHlms(Ogre::HLMS_PBS)));

    Ogre::HlmsPbs* hlmsPbs = static_cast<Ogre::HlmsPbs*>(hlmsManager->getHlms(Ogre::HLMS_PBS));

    std::string strBlockName(srcMtl.name);
    Ogre::HlmsPbsDatablock* datablock = nullptr;

    try
    {
        datablock = static_cast<Ogre::HlmsPbsDatablock*>(
            hlmsPbs->createDatablock(strBlockName, strBlockName,
                                     Ogre::HlmsMacroblock(),
                                     Ogre::HlmsBlendblock(),
                                     Ogre::HlmsParamVec()));
    }
    catch (std::exception& e)
    {
        qDebug() << "Cannot create datablock." << e.what();
        return nullptr;
    }

    Ogre::HlmsSamplerblock samplerBlock;
    samplerBlock.setAddressingMode(Ogre::TAM_WRAP);
    samplerBlock.setFiltering(Ogre::TFO_ANISOTROPIC);

    datablock->setWorkflow(Ogre::HlmsPbsDatablock::MetallicWorkflow);

    auto envMap = hlmsTextureManager->createOrRetrieveTexture("env.dds", Ogre::HlmsTextureManager::TEXTURE_TYPE_ENV_MAP);
    datablock->setTexture(Ogre::PBSM_REFLECTION, envMap.xIdx, envMap.texture);

    datablock->setDiffuse(Ogre::Vector3(srcMtl.diffuse[0], srcMtl.diffuse[1], srcMtl.diffuse[2]));
    datablock->setBackgroundDiffuse(Ogre::ColourValue(1, 1, 1, 1));
    datablock->setSpecular(Ogre::Vector3(srcMtl.specular[0], srcMtl.specular[1], srcMtl.specular[2]));
    datablock->setRoughness(srcMtl.roughness);
    datablock->setMetalness(srcMtl.metallic);

    if (!srcMtl.diffuse_texname.empty())
    {
        auto tex = hlmsTextureManager->createOrRetrieveTexture(srcMtl.diffuse_texname, Ogre::HlmsTextureManager::TEXTURE_TYPE_DIFFUSE);
        datablock->setTexture(Ogre::PBSM_DIFFUSE, tex.xIdx, tex.texture);
        datablock->setSamplerblock(Ogre::PBSM_DIFFUSE, samplerBlock);
    }

    if (!srcMtl.specular_texname.empty())
    {
        auto tex = hlmsTextureManager->createOrRetrieveTexture(srcMtl.specular_texname, Ogre::HlmsTextureManager::TEXTURE_TYPE_DIFFUSE);
        datablock->setTexture(Ogre::PBSM_SPECULAR, tex.xIdx, tex.texture);
        datablock->setSamplerblock(Ogre::PBSM_SPECULAR, samplerBlock);
    }

    if (!srcMtl.roughness_texname.empty())
    {
        auto tex = hlmsTextureManager->createOrRetrieveTexture(srcMtl.roughness_texname, Ogre::HlmsTextureManager::TEXTURE_TYPE_MONOCHROME);
        datablock->setTexture(Ogre::PBSM_ROUGHNESS, tex.xIdx, tex.texture);
        datablock->setSamplerblock(Ogre::PBSM_ROUGHNESS, samplerBlock);
    }

    if (!srcMtl.metallic_texname.empty())
    {
        auto tex = hlmsTextureManager->createOrRetrieveTexture(srcMtl.metallic_texname, Ogre::HlmsTextureManager::TEXTURE_TYPE_MONOCHROME);
        datablock->setTexture(Ogre::PBSM_METALLIC, tex.xIdx, tex.texture);
        datablock->setSamplerblock(Ogre::PBSM_METALLIC, samplerBlock);
    }

    if (!srcMtl.normal_texname.empty())
    {
        auto tex = hlmsTextureManager->createOrRetrieveTexture(srcMtl.normal_texname, Ogre::HlmsTextureManager::TEXTURE_TYPE_NORMALS);
        datablock->setTexture(Ogre::PBSM_NORMAL, tex.xIdx, tex.texture);
        datablock->setSamplerblock(Ogre::PBSM_NORMAL, samplerBlock);
    }

    Ogre::HlmsJsonPbs hlmsJson(hlmsManager, nullptr, "");
    std::string outJson;
    hlmsJson.saveMaterial(datablock, outJson); // FIXME:

    //std::ofstream fout("C:/Users/Matt/Desktop/abc.material.json", std::ofstream::out);
    //fout << outJson;
    //fout.close();

    return datablock;
}
