
#include "stdafx.h"

#include "objexporter.h"

#include "OgreMesh.h"
#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreSubMesh2.h"

#include "OgreHlmsPbs.h"
#include "OgreHlmsPbsDatablock.h"

#include "OgreXML/OgreXMLMeshSerializer.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QXmlStreamReader>
#include <QVector3D>
#include <QTemporaryDir>

#define CLOCK_LINENUM_CAT( name, ln ) name##ln
#define CLOCK_LINENUM( name, ln ) CLOCK_LINENUM_CAT( name, ln )
#define CLOCK_BEGIN CLOCK_LINENUM(t1, __LINE__)
#define CLOCK_END   CLOCK_LINENUM(t2, __LINE__)
#define PROFILE( f ) \
    clock_t CLOCK_BEGIN = clock(); \
    f; \
    clock_t CLOCK_END = clock(); \
    qDebug() << #f << ": Use" << float( CLOCK_END - CLOCK_BEGIN ) / CLOCKS_PER_SEC << "sec";


ObjExporter::ObjExporter()
{
}

bool ObjExporter::writeToFile(Ogre::Mesh* srcMesh, const QString& sOutFile)
{
    Q_ASSERT(srcMesh);
    auto& meshV1Mgr = Ogre::v1::MeshManager::getSingleton();

    static int convertionCount = 4431; // just to avoid name conflicts
    std::ostringstream sout;
    sout << "conversion_" << convertionCount++;

    Ogre::v1::MeshPtr v1Mesh = meshV1Mgr.createManual(sout.str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    v1Mesh->importV2(srcMesh);

    QTemporaryDir tempDir;
    if (!tempDir.isValid())
    {
        return false;
    }

    //QString sTempXmlFile = "C:/Users/Matt/Desktop/abc.mesh.xml";
    QString sTempXmlFile = QDir(tempDir.path()).filePath("out.mesh.xml");

    Ogre::v1::XMLMeshSerializer xmlSerializer;
    PROFILE(xmlSerializer.exportMesh(v1Mesh.get(), sTempXmlFile.toStdString()));
    PROFILE(convertToOgreData(sTempXmlFile));

    QFileInfo info(sOutFile);
    QString sMtlFile = info.absoluteDir().filePath(info.baseName() + ".mtl");
    PROFILE(writeMtlFile(srcMesh, sMtlFile));
    PROFILE(writeObjFile(sOutFile, info.baseName() + ".mtl"));

    qDebug() << "Output Obj File=" << sOutFile;

    meshV1Mgr.remove(v1Mesh);

    bool b = QFile::remove(sTempXmlFile);
    Q_ASSERT(b);

    return true;
}

bool ObjExporter::convertToOgreData(const QString& sXmlFile)
{
    QFile file(sXmlFile);
    if (file.open(QIODevice::ReadOnly) == false)
        return false;

    QXmlStreamReader xin;
    xin.setDevice(&file);

    OgreDataSubMesh currentMesh;
    OgreDataVertex currentVertex;

    while (!xin.atEnd())
    {
        xin.readNext();
        //auto theType = xin.readNext();
        //qDebug() << theType;
        //qDebug() << "Type=" << theType << "Name=" << xin.name().toString();

        if (xin.isStartElement())
        {
            const QStringRef tagName = xin.name();
            if (tagName == "submesh")
            {
                clearOgreDataSubMesh(currentMesh);
                currentMesh.material = xin.attributes().value("material").toString().toStdString();
            }
            else if (tagName == "face")
            {
                OgreDataFace f;
                f.index[0] = xin.attributes().value("v1").toInt();
                f.index[1] = xin.attributes().value("v2").toInt();
                f.index[2] = xin.attributes().value("v3").toInt();
                //qDebug() << f.index[0] << f.index[1] << f.index[2];
                currentMesh.faces.push_back(f);
            }
            else if (tagName == "faces")
            {
                qDebug() << "Face Count=" << xin.attributes().value("count");
            }
            else if (tagName == "vertex")
            {
                clearOgreDataVertex(currentVertex);
            }
            else if (tagName == "position")
            {
                currentVertex.position[0] = xin.attributes().value("x").toFloat();
                currentVertex.position[1] = xin.attributes().value("y").toFloat();
                currentVertex.position[2] = xin.attributes().value("z").toFloat();
            }
            else if (tagName == "normal")
            {
                currentVertex.normal[0] = xin.attributes().value("x").toFloat();
                currentVertex.normal[1] = xin.attributes().value("y").toFloat();
                currentVertex.normal[2] = xin.attributes().value("z").toFloat();
            }
            else if (tagName == "texcoord")
            {
                currentVertex.texcoord[0] = xin.attributes().value("u").toFloat();
                currentVertex.texcoord[1] = xin.attributes().value("v").toFloat();
            }
        }
        else if (xin.isEndElement())
        {
            const QStringRef tagName = xin.name();
            if (tagName == "submesh")
            {
                mSubmeshes.push_back(currentMesh);
            }
            else if (tagName == "vertex")
            {
                normalize(currentVertex);
                currentMesh.vertices.push_back(currentVertex);
            }
        }
    }

    qDebug() << "Mesh count =" << mSubmeshes.size();
    for (const OgreDataSubMesh& m : mSubmeshes)
    {
        qDebug() << "face count=" << m.faces.size();
        qDebug() << "vertex count=" << m.vertices.size();
    }

    if (xin.hasError())
    {
        qDebug() << "ERROR:" << xin.errorString();
        return false;
    }

    return true;
}

void ObjExporter::clearOgreDataSubMesh(OgreDataSubMesh& m)
{
    m.meshName = "";
    m.material = "";
    m.faces.clear();
    m.vertices.clear();
}

void ObjExporter::clearOgreDataVertex(OgreDataVertex& v)
{
    v.position[0] = 0.f;
    v.position[1] = 0.f;
    v.position[2] = 0.f;

    v.normal[0] = 0.f;
    v.normal[1] = 0.f;
    v.normal[2] = 0.f;

    v.texcoord[0] = 0.f;
    v.texcoord[1] = 0.f;
}

bool ObjExporter::writeObjFile(const QString& sOutFile, const QString& sMtlFileName)
{
    QFile file(sOutFile);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
    {
        return false;
    }
    int64_t indexOffset = 1; // obj uses 1 based index

    QTextStream fout(&file);

    fout << "mtllib " << sMtlFileName << "\n";

    for (int i = 0; i < mSubmeshes.size(); ++i)
    {
        OgreDataSubMesh& mesh01 = mSubmeshes[i];
        if (!mesh01.meshName.empty())
            fout << "o " << QString::fromStdString(mesh01.meshName) << "\n";
        else
            fout << "o exportMesh001\n";

        // write vertices
        for (const OgreDataVertex& v : mesh01.vertices)
        {

            QString sout;
            fout << "v " << qSetRealNumberPrecision(6)
                << v.position[0] << " "
                << v.position[1] << " "
                << v.position[2] << "\n";
            //fout << QString::asprintf("v %.6f %.6f %.6f\n", v.position[0], v.position[1], v.position[2]);
            //fout << sout;
        }
        for (const OgreDataVertex& v : mesh01.vertices)
        {
            //QString sout;
            fout << QString::asprintf("vn %.6f %.6f %.6f\n", v.normal[0], v.normal[1], v.normal[2]);
            //fout << sout;
        }
        for (const OgreDataVertex& v : mesh01.vertices)
        {
            //QString sout;
            fout << QString::asprintf("vt %.4f %.4f\n", v.texcoord[0], v.texcoord[1]);
            //fout << sout;
        }

        // TODO: usemtl
        fout << "usemtl " << mesh01.material.c_str() << "\n";

        // write faces
        for (const OgreDataFace& f : mesh01.faces)
        {
            fout << "f ";
            fout << (f.index[0] + indexOffset) << "/"
                << (f.index[0] + indexOffset) << "/"
                << (f.index[0] + indexOffset) << " ";

            fout << (f.index[1] + indexOffset) << "/"
                << (f.index[1] + indexOffset) << "/"
                << (f.index[1] + indexOffset) << " ";

            fout << (f.index[2] + indexOffset) << "/"
                << (f.index[2] + indexOffset) << "/"
                << (f.index[2] + indexOffset) << "\n";
        }

        fout.flush();

        indexOffset += mesh01.vertices.size();
    }

    file.flush();
    file.close();

    return true;
}

bool ObjExporter::writeMtlFile(Ogre::Mesh* mesh01, const QString& sOutFile)
{
    /*
    Ogre::Root& root = Ogre::Root::getSingleton();
    Ogre::HlmsManager* hlmsManager = root.getHlmsManager();
    Ogre::HlmsTextureManager* hlmsTextureManager = hlmsManager->getTextureManager();
    Q_ASSERT(dynamic_cast<Ogre::HlmsPbs*>(hlmsManager->getHlms(Ogre::HLMS_PBS)));

    Ogre::HlmsPbs* hlmsPbs = static_cast<Ogre::HlmsPbs*>(hlmsManager->getHlms(Ogre::HLMS_PBS));

    QFileInfo info(sOutFile);
    std::string sOutFolder = info.absolutePath().toStdString() + "/";

    QFile file(sOutFile);
    if (!file.open(QIODevice::WriteOnly))
    {
        qDebug() << "Error: unable to write to file " << sOutFile;
        return false;
    }

    QTextStream fout(&file);

    for (int m = 0; m < mesh01->getNumSubMeshes(); ++m)
    {
        Ogre::SubMesh* subMesh = mesh01->getSubMesh(m);
        //Ogre::HlmsPbsDatablock* dataBlock = subMesh->getD
        qDebug() << "Mtl: " << subMesh->getMaterialName().c_str();

        auto datablock = (Ogre::HlmsPbsDatablock*)hlmsPbs->getDatablock(subMesh->getMaterialName());
        if (datablock == nullptr)
        {
            continue;
        }

        fout << "newmtl " << subMesh->getMaterialName().c_str() << "\n";

        Ogre::ColourValue backgrond = datablock->getBackgroundDiffuse();
        fout << QString::asprintf("Ka %.3f %.3f %.3f\n", backgrond.r, backgrond.g, backgrond.b);

        Ogre::Vector3 diffuse = datablock->getDiffuse();
        fout << QString::asprintf("Kd %.3f %.3f %.3f\n", diffuse.x, diffuse.y, diffuse.z);

        Ogre::Vector3 speclr = datablock->getSpecular();
        fout << QString::asprintf("Ks %.3f %.3f %.3f\n", speclr.x, speclr.y, speclr.z);

        qDebug() << "It's not able to export textures at the moment.";
        
        //Ogre::TexturePtr diffuseTex = datablock->getTexture(Ogre::PBSM_DIFFUSE);
        //writeTexture(diffuseTex.get(), sOutFolder + "diffuse.png");

        //Ogre::TexturePtr normalTex = datablock->getTexture(Ogre::PBSM_NORMAL);
        //writeTexture(normalTex.get(), sOutFolder + "normal.png");

        //Ogre::TexturePtr roughnessTex = datablock->getTexture(Ogre::PBSM_ROUGHNESS);
        //writeTexture(roughnessTex.get(), sOutFolder + "roughness.png");

        //Ogre::TexturePtr metallicTex = datablock->getTexture(Ogre::PBSM_METALLIC);
        //writeTexture(metallicTex.get(), sOutFolder + "metallic.png");
        

        float trans = datablock->getTransparency();
        fout << QString::asprintf("d  %.4f\n", trans);
        fout << QString::asprintf("Tr %.4f\n", (1.0 - trans));

        fout << "illum 2\n";
        fout << QString::asprintf("Pm %.4f\n", datablock->getMetalness());
        fout << QString::asprintf("Pr %.4f\n", datablock->getRoughness());
    }

    fout.flush();
    file.close();
    */
    return true;
}

void ObjExporter::writeTexture(Ogre::TextureGpu* tex, const std::string& sTexFileName)
{
    if (tex)
    {
        try
        {
            //qDebug() << "Array: " << tex->isTextureTypeArray();
            Ogre::Image2 img;
            img.convertFromTexture(tex, 0, 0);
            img.save(sTexFileName, 0, 1);
        }
        catch (const std::exception&)
        {
            qDebug() << "Failed to write image:" << sTexFileName.c_str();
        }
    }
}

void ObjExporter::normalize(OgreDataVertex& v)
{
    QVector3D v3D(v.normal[0], v.normal[1], v.normal[2]);
    v3D.normalize();

    if (isnan(v3D.x()))
    {
        v.normal[0] = 0.0f;
        v.normal[1] = 1.0f;
        v.normal[2] = 0.0f;
        return;
    }

    v.normal[0] = v3D.x();
    v.normal[1] = v3D.y();
    v.normal[2] = v3D.z();
}
