
#include "stdafx.h"
#include "objexporter.h"
#include "OgreXMLMeshSerializer.h"

#include "OgreMesh.h"
#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"

#include <QFile>
#include <QXmlStreamReader>
#include <QVector3D>

#define CLOCK_LINENUM_CAT( name, ln ) name##ln
#define CLOCK_LINENUM( name, ln ) CLOCK_LINENUM_CAT( name, ln )
#define PROFILE( f ) \
    clock_t CLOCK_LINENUM(t1, __LINE__) = clock(); \
    f; \
    clock_t CLOCK_LINENUM(t2, __LINE__) = clock(); \
    qDebug() << #f << ": Use" << float( CLOCK_LINENUM(t2, __LINE__) - CLOCK_LINENUM(t1, __LINE__) ) / CLOCKS_PER_SEC << "sec";


ObjExporter::ObjExporter()
{
}

bool ObjExporter::exportFile(Ogre::Mesh* srcMesh, const QString& sOutFile)
{
    Q_ASSERT(srcMesh);
    auto& meshV1Mgr = Ogre::v1::MeshManager::getSingleton();

    static int convertionCount = 4431; // just to avoid name conflicts
    std::ostringstream sout;
    sout << "conversion_" << convertionCount++;

    Ogre::v1::MeshPtr v1Mesh = meshV1Mgr.createManual(sout.str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    v1Mesh->importV2(srcMesh);

    QString sTempXmlFile = "C:/Users/Matt/Desktop/abc.mesh.xml";

    Ogre::v1::XMLMeshSerializer xmlSerializer;
    PROFILE( xmlSerializer.exportMesh(v1Mesh.get(), sTempXmlFile.toStdString()) );

    PROFILE(convertToOgreData(sTempXmlFile));

    PROFILE(writeObjFile(sOutFile));

    qDebug() << "Output Obj File=" << sOutFile;
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
        auto theType = xin.readNext();
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
    for (OgreDataSubMesh& m : mSubmeshes)
    {
        qDebug() << "face count=" << m.faces.size();
        qDebug() << "vertex count=" << m.vertices.size();
    }

    if ( xin.hasError() )
        qDebug() << "ERROR:" << xin.errorString();
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
    memset(&v, 0, sizeof(OgreDataVertex));
}

bool ObjExporter::writeObjFile(const QString& sOutFile)
{
    QFile file(sOutFile);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
    {
        return false;
    }
    int64_t indexOffset = 1; // obj uses 1 based index

    QTextStream fout(&file);

    fout << "mtllib " << "haha.mtl" << "\n";
        
    for (int i = 0; i < mSubmeshes.size(); ++i)
    {
        OgreDataSubMesh& mesh01 = mSubmeshes[i];
        fout << "o " << mesh01.meshName.c_str() << "\n";
            
        // write vertices
        for (const OgreDataVertex& v : mesh01.vertices )
        {
            QString sout;
            sout.sprintf("v %.6f %.6f %.6f\n", v.position[0], v.position[1], v.position[2]);
            fout << sout;
        }
        for (const OgreDataVertex& v : mesh01.vertices)
        {
            QString sout;
            sout.sprintf("vn %.6f %.6f %.6f\n", v.normal[0], v.normal[1], v.normal[2]);
            fout << sout;
        }
        for (const OgreDataVertex& v : mesh01.vertices)
        {
            QString sout;
            sout.sprintf("vt %.4f %.4f\n", v.texcoord[0], v.texcoord[1]);
            fout << sout;
        }

        // TODO: usemtl

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

bool ObjExporter::writeMtlFile(Ogre::Mesh*, const QString& sOutFile)
{
    return true;
}

void ObjExporter::normalize(OgreDataVertex& v)
{
    QVector3D v3D(v.normal[0], v.normal[1], v.normal[2]);
    v3D.normalize();

    v.normal[0] = v3D.x();
    v.normal[1] = v3D.y();
    v.normal[2] = v3D.z();
}
