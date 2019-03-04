
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


bool operator<(const UniqueVertex& l, const UniqueVertex& r)
{
    return std::tie(l.v, l.n, l.t) < std::tie(r.v, r.n, r.t);
}

Ogre::HlmsPbsDatablock* importMaterial(const tinyobj::material_t& srcMtl)
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

ObjImporter::ObjImporter()
{}

bool ObjImporter::import(const QString& sObjFile, const QString& sOgreMeshFile)
{
    QFileInfo info(sObjFile);

    QProgressDialog progress(nullptr, Qt::Dialog | Qt::WindowTitleHint);
    progress.setLabelText(QString("Converting %1...").arg(info.fileName()));
    progress.setRange(0, 100);
    progress.setModal(true);
    progress.show();
    QApplication::processEvents();

    std::string sError;
    std::string sMtlBasePath = info.absolutePath().toStdString() + "/";
    PROFILE(bool b = tinyobj::LoadObj(&mObjAttrib, &mObjShapes, &mObjMaterials, &sError, sObjFile.toStdString().c_str(), sMtlBasePath.c_str(), true));
    Q_ASSERT(b);

    if (!sError.empty())
    {
        qDebug() << "Error:" << sError.c_str();
    }

    progress.setValue(10);
    QApplication::processEvents();

    for (tinyobj::material_t& mtl : mObjMaterials)
    {
        if (mImportedMaterials.count(mtl.name) == 0)
        {
            importMaterial(mtl);
            mImportedMaterials.insert(mtl.name);
        }
    }

    progress.setValue(20);
    QApplication::processEvents();

    //PROFILE(PreprocessObjIndexes());

    progress.setValue(30);
    QApplication::processEvents();

    PROFILE(convertToOgreData());

    progress.setValue(40);
    QApplication::processEvents();

    QTemporaryDir tempDir;
    if (!tempDir.isValid())
    {
        return false;
    }

    QString strTempXMLFile = QDir(tempDir.path()).filePath("out.mesh.xml");
    //QString strTempXMLFile = "C:/Users/Matt/Desktop/out.mesh.xml";
    PROFILE(writeMeshXML(strTempXMLFile));

    progress.setValue(60);
    QApplication::processEvents();

    Ogre::v1::MeshPtr v1MeshPtr;
    PROFILE(importOgreMeshFromXML(strTempXMLFile, v1MeshPtr));

    progress.setValue(80);
    QApplication::processEvents();

    auto v2Mesh = Ogre::MeshManager::getSingleton().createManual("v2Mesh", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    v2Mesh->importV1(v1MeshPtr.get(), false, false, true);

    Ogre::Root* root = Ogre::Root::getSingletonPtr();
    Ogre::MeshSerializer meshSerializer2(root->getRenderSystem()->getVaoManager());
    meshSerializer2.exportMesh(v2Mesh.get(), sOgreMeshFile.toStdString());

    progress.setValue(90);
    QApplication::processEvents();

    Ogre::MeshManager::getSingleton().remove(v2Mesh);
    Ogre::v1::MeshManager::getSingleton().remove(v1MeshPtr);
    v2Mesh.reset();
    v1MeshPtr.reset();

    return b;
}

void ObjImporter::writeMeshXML(const QString& sOutFile)
{
    qDebug() << "  Temp mesh.xml=" << sOutFile;

    QFile outputFile(sOutFile);
    bool b = outputFile.open(QFile::WriteOnly);
    Q_ASSERT(b);

    QXmlStreamWriter xout(&outputFile);
#ifdef _DEBUG
    xout.setAutoFormatting(true);
#endif
    xout.writeStartDocument();
    xout.writeStartElement("mesh");
    {
        xout.writeStartElement("submeshes");
        for (int s = 0; s < mOgreSubMeshes.size(); ++s)
        {
            writeXMLOneSubMesh(xout, mOgreSubMeshes[s]);
            QApplication::processEvents();
        }
        xout.writeEndElement(); // submeshes

        xout.writeStartElement("submeshnames");
        {
            for (int s = 0; s < mOgreSubMeshes.size(); ++s)
            {
                //<submeshname name="Cerberus00_Fixed0" index="0" />
                xout.writeStartElement("submeshname");
                xout.writeAttribute("name", QString::fromStdString(mOgreSubMeshes[s].meshName));
                xout.writeAttribute("index", QString::number(s));
                xout.writeEndElement();

                QApplication::processEvents();
            }
        }
        xout.writeEndElement();
    }
    xout.writeEndElement();
    xout.writeEndDocument();

    outputFile.flush();
    outputFile.close();

    mOgreSubMeshes.clear();
}

void ObjImporter::writeXMLOneSubMesh(QXmlStreamWriter& xout, const OgreDataSubMesh& mesh01)
{
    // <submesh material="pistol_fbx#0" usesharedvertices="false" use32bitindexes="true" operationtype="triangle_list">
    xout.writeStartElement("submesh");
    {
        xout.writeAttribute("material", QString::fromStdString(mesh01.material));
        xout.writeAttribute("usesharedvertices", "false");
        xout.writeAttribute("use32bitindexes", "true");
        xout.writeAttribute("operationtype", "triangle_list");

        writeXMLFaces(xout, mesh01);
        writeXMLGeometry(xout, mesh01);
    }
    xout.writeEndElement(); // submesh
}

void ObjImporter::writeXMLFaces(QXmlStreamWriter& xout, const OgreDataSubMesh& mesh01)
{
    xout.writeStartElement("faces");
    {
        xout.writeAttribute("count", QString::number(mesh01.faces.size()));

        for (const OgreDataFace& f : mesh01.faces)
        {
            xout.writeStartElement("face");
            xout.writeAttribute("v1", QString::number(f.index[0]));
            xout.writeAttribute("v2", QString::number(f.index[1]));
            xout.writeAttribute("v3", QString::number(f.index[2]));
            xout.writeEndElement(); // face
        }
    }
    xout.writeEndElement(); // faces
}

void ObjImporter::writeXMLGeometry(QXmlStreamWriter& xout, const OgreDataSubMesh& mesh01)
{
    xout.writeStartElement("geometry");
    {
        xout.writeAttribute("vertexcount", QString::number(mesh01.vertices.size()));

        // <vertexbuffer positions="true" normals="true" texture_coord_dimensions_0="float2" texture_coords="1">
        xout.writeStartElement("vertexbuffer");
        {
            xout.writeAttribute("positions", "true");
            xout.writeAttribute("normals", "true");
            xout.writeAttribute("texture_coord_dimensions_0", "float2");
            xout.writeAttribute("texture_coords", "1");

            //<vertex>
            //  <position x="-3.43088" y="-10.7253" z="23.8002" />
            //  <normal   x="-0.84353" y="0.45537" z="0.284766" />
            //  <texcoord u="0.169636" v="0.699241" />
            //</vertex>
            for (int k = 0; k < mesh01.vertices.size(); ++k)
            {
                const OgreDataVertex& v01 = mesh01.vertices[k];
                xout.writeStartElement("vertex");
                {
                    // ---- position ----
                    xout.writeStartElement("position");
                    xout.writeAttribute("x", QString::number(v01.position[0]));
                    xout.writeAttribute("y", QString::number(v01.position[1]));
                    xout.writeAttribute("z", QString::number(v01.position[2]));
                    xout.writeEndElement();

                    // ---- normal ----
                    xout.writeStartElement("normal");
                    xout.writeAttribute("x", QString::number(v01.normal[0]));
                    xout.writeAttribute("y", QString::number(v01.normal[1]));
                    xout.writeAttribute("z", QString::number(v01.normal[2]));
                    xout.writeEndElement();

                    // ---- texcoord ----
                    xout.writeStartElement("texcoord");
                    xout.writeAttribute("u", QString::number(v01.texcoord[0]));
                    xout.writeAttribute("v", QString::number(v01.texcoord[1]));
                    xout.writeEndElement();
                }
                xout.writeEndElement(); // vertex
            }
        }
        xout.writeEndElement(); // vertex buffer
    }
    xout.writeEndElement(); // geometry
}

void ObjImporter::convertToOgreData()
{
    mOgreSubMeshes.clear();

    for (int s = 0; s < mObjShapes.size(); ++s)
    {
        tinyobj::mesh_t& mesh01 = mObjShapes[s].mesh;
        qDebug() << "    Converting Mesh=" << mObjShapes[s].name.c_str();
        qDebug() << "      face count=" << mesh01.indices.size() / 3;

        // build indexes
        std::set<UniqueVertex> uniqueVerticesSet;
        Q_ASSERT(mesh01.indices.size() % 3 == 0);
        for (auto& i : mesh01.indices)
        {
            auto p = uniqueVerticesSet.insert(UniqueVertex(i));
        }

        Q_ASSERT(mUniqueVerticesVec.empty());
        mUniqueVerticesVec.assign(uniqueVerticesSet.begin(), uniqueVerticesSet.end());

        uniqueVerticesSet.clear();

        Q_ASSERT(mUniqueVerticesIndexMap.empty());
        for (int i = 0; i < mUniqueVerticesVec.size(); ++i)
        {
            mUniqueVerticesIndexMap.emplace(mUniqueVerticesVec[i], i);
        }

        OgreDataSubMesh subMesh = convertObjMeshToOgreData(mesh01);

        if (subMesh.bNeedGenerateNormals)
        {
            generateNormalVectors(OUT subMesh);
        }

        if (mZUpToYUp)
        {
            convertFromZUpToYUp(subMesh);
        }

        if (mesh01.material_ids[0] >= 0)
        {
            subMesh.material = mObjMaterials[mesh01.material_ids[0]].name;
        }
        subMesh.meshName = mObjShapes[s].name;

        mOgreSubMeshes.push_back(subMesh);


        mUniqueVerticesVec.clear();
        mUniqueVerticesIndexMap.clear();

        QApplication::processEvents();
    }
}

ObjImporter::OgreDataSubMesh ObjImporter::convertObjMeshToOgreData(const tinyobj::mesh_t& mesh01)
{
    OgreDataSubMesh mout;

    Q_ASSERT(mesh01.indices.size() % 3 == 0);

    // convert faces
    for (int f = 0; f < mesh01.indices.size(); f += 3)
    {
        int v1 = mUniqueVerticesIndexMap.at(UniqueVertex(mesh01.indices[f + 0]));
        int v2 = mUniqueVerticesIndexMap.at(UniqueVertex(mesh01.indices[f + 1]));
        int v3 = mUniqueVerticesIndexMap.at(UniqueVertex(mesh01.indices[f + 2]));

        mout.faces.push_back(OgreDataFace{ v1, v2, v3 });
    }

    // convert vertex buffer
    for (int k = 0; k < mUniqueVerticesVec.size(); ++k)
    {
        UniqueVertex& uniqueV = mUniqueVerticesVec[k];

        OgreDataVertex v1;
        {
            float posX = mObjAttrib.vertices[uniqueV.v * 3 + 0];
            float posY = mObjAttrib.vertices[uniqueV.v * 3 + 1];
            float posZ = mObjAttrib.vertices[uniqueV.v * 3 + 2];

            v1.position[0] = posX;
            v1.position[1] = posY;
            v1.position[2] = posZ;

            if (uniqueV.n != -1)
            {
                float normalX = mObjAttrib.normals[uniqueV.n * 3 + 0];
                float normalY = mObjAttrib.normals[uniqueV.n * 3 + 1];
                float normalZ = mObjAttrib.normals[uniqueV.n * 3 + 2];
                v1.normal[0] = normalX;
                v1.normal[1] = normalY;
                v1.normal[2] = normalZ;
            }
            else
            {
                mout.bNeedGenerateNormals = true;
            }

            if (uniqueV.t != -1)
            {
                float texCoordU = mObjAttrib.texcoords[uniqueV.t * 2 + 0];
                float texCoordV = mObjAttrib.texcoords[uniqueV.t * 2 + 1];
                v1.texcoord[0] = texCoordU;
                v1.texcoord[1] = 1.0 - texCoordV;
            }
            else
            {
                v1.texcoord[0] = 0;
                v1.texcoord[1] = 0;
            }
        }
        mout.vertices.push_back(v1);
    }

    return mout;
}

void ObjImporter::importOgreMeshFromXML(const QString& sXMLFile, Ogre::v1::MeshPtr& meshV1Ptr)
{
    static int convertionCount = 0; // just to avoid name conflicts
    std::ostringstream sout;
    sout << "conversion_" << convertionCount++;
    meshV1Ptr = Ogre::v1::MeshManager::getSingleton().createManual(
        sout.str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

    std::string source = sXMLFile.toStdString();

    Ogre::v1::XMLMeshSerializer xmlMeshSerializer;
    Ogre::VertexElementType colourElementType = Ogre::VET_COLOUR_ABGR;
    xmlMeshSerializer.importMesh(source, colourElementType, meshV1Ptr.get());

    // Make sure animation types are up to date first
    meshV1Ptr->_determineAnimationTypes();
    meshV1Ptr->buildTangentVectors();
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
