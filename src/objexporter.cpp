
#include "stdafx.h"

#include "objexporter.h"

#include "OgreHlmsPbsDatablock.h"
#include "OgreImage2.h"
#include "OgreMatrix3.h"
#include "textureutil.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>


bool ObjExporter::writeToFile(const std::vector<MeshData::Object>& objects, const QString& sOutFile)
{
    QFileInfo info(sOutFile);
    const QString sMtlFileName = info.completeBaseName() + ".mtl";

    if (!writeMtlFile(objects, info.absoluteDir().filePath(sMtlFileName)))
        return false;

    return writeObjFile(objects, sOutFile, sMtlFileName);
}

QString ObjExporter::materialName(const MeshData::SubMesh& subMesh)
{
    QString name = QString::fromStdString(subMesh.materialName);
    if (name.isEmpty())
        name = "default";

    // Names in .obj/.mtl files end at the first whitespace
    name.replace(QRegularExpression("\\s"), "_");
    return name;
}

bool ObjExporter::writeObjFile(const std::vector<MeshData::Object>& objects, const QString& sOutFile, const QString& sMtlFileName)
{
    QFile file(sOutFile);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
    {
        qDebug() << "Error: unable to write to file " << sOutFile;
        return false;
    }

    QTextStream fout(&file);
    fout << "mtllib " << sMtlFileName << "\n";

    int64_t indexOffset = 1; // obj uses 1 based index

    for (const MeshData::Object& object : objects)
    {
        Ogre::Matrix3 normalMatrix;
        object.worldTransform.extract3x3Matrix(normalMatrix);
        normalMatrix = normalMatrix.Inverse().Transpose();

        QString objectName = QString::fromStdString(object.name);
        objectName.replace(QRegularExpression("\\s"), "_");

        for (size_t i = 0; i < object.subMeshes.size(); ++i)
        {
            const MeshData::SubMesh& subMesh = object.subMeshes[i];
            if (subMesh.vertices.empty())
                continue;

            fout << "o " << objectName << "_" << i << "\n";

            for (const MeshData::Vertex& v : subMesh.vertices)
            {
                const Ogre::Vector3 p = object.worldTransform * Ogre::Vector3(v.position[0], v.position[1], v.position[2]);
                fout << QString::asprintf("v %.6f %.6f %.6f\n", p.x, p.y, p.z);
            }
            for (const MeshData::Vertex& v : subMesh.vertices)
            {
                Ogre::Vector3 n = normalMatrix * Ogre::Vector3(v.normal[0], v.normal[1], v.normal[2]);
                if (n.normalise() == 0.0f || n.isNaN())
                    n = Ogre::Vector3::UNIT_Y;
                fout << QString::asprintf("vn %.6f %.6f %.6f\n", n.x, n.y, n.z);
            }
            for (const MeshData::Vertex& v : subMesh.vertices)
            {
                // obj has v = 0 at the bottom, Ogre at the top
                fout << QString::asprintf("vt %.6f %.6f\n", v.texcoord[0], 1.0f - v.texcoord[1]);
            }

            fout << "usemtl " << materialName(subMesh) << "\n";

            for (size_t f = 0; f + 2 < subMesh.indices.size(); f += 3)
            {
                fout << "f";
                for (int k = 0; k < 3; ++k)
                {
                    const int64_t index = subMesh.indices[f + k] + indexOffset;
                    fout << " " << index << "/" << index << "/" << index;
                }
                fout << "\n";
            }

            indexOffset += int64_t(subMesh.vertices.size());
        }
    }

    fout.flush();
    file.close();
    return true;
}

bool ObjExporter::writeMtlFile(const std::vector<MeshData::Object>& objects, const QString& sOutFile)
{
    QFile file(sOutFile);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
    {
        qDebug() << "Error: unable to write to file " << sOutFile;
        return false;
    }

    const QFileInfo info(sOutFile);
    const QString sFolder = info.absolutePath();

    QTextStream fout(&file);
    QSet<QString> written;

    for (const MeshData::Object& object : objects)
    {
        for (const MeshData::SubMesh& subMesh : object.subMeshes)
        {
            const QString name = materialName(subMesh);
            if (subMesh.vertices.empty() || written.contains(name))
                continue;
            written.insert(name);

            fout << "newmtl " << name << "\n";

            auto datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(subMesh.datablock);
            if (datablock == nullptr)
            {
                fout << "Kd 1.000 1.000 1.000\n\n";
                continue;
            }

            const Ogre::Vector3 diffuse = datablock->getDiffuse();
            fout << QString::asprintf("Kd %.3f %.3f %.3f\n", diffuse.x, diffuse.y, diffuse.z);

            const Ogre::Vector3 specular = datablock->getSpecular();
            fout << QString::asprintf("Ks %.3f %.3f %.3f\n", specular.x, specular.y, specular.z);

            const Ogre::Vector3 emissive = datablock->getEmissive();
            fout << QString::asprintf("Ke %.3f %.3f %.3f\n", emissive.x, emissive.y, emissive.z);

            const float alpha = datablock->getTransparency();
            fout << QString::asprintf("d %.4f\n", alpha);

            fout << "illum 2\n";
            fout << QString::asprintf("Pr %.4f\n", datablock->getRoughness());
            fout << QString::asprintf("Pm %.4f\n", datablock->getMetalness());

            const struct { Ogre::PbsTextureTypes type; const char* keyword; const char* suffix; } maps[] = {
                { Ogre::PBSM_DIFFUSE, "map_Kd", "diffuse" },
                { Ogre::PBSM_ROUGHNESS, "map_Pr", "roughness" },
                { Ogre::PBSM_METALLIC, "map_Pm", "metallic" },
                { Ogre::PBSM_EMISSIVE, "map_Ke", "emissive" },
            };
            for (const auto& map : maps)
            {
                const QString texFile = writeTexture(datablock->getTexture(map.type), sFolder, name + "_" + map.suffix);
                if (!texFile.isEmpty())
                    fout << map.keyword << " " << texFile << "\n";
            }
            fout << "\n";
        }
    }

    fout.flush();
    file.close();
    return true;
}

QString ObjExporter::writeTexture(Ogre::TextureGpu* tex, const QString& sFolder, const QString& sBaseName)
{
    if (tex == nullptr)
        return QString();

    QString fileName = sBaseName + ".png";
    fileName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");

    try
    {
        Ogre::Image2 img;
        if (!TextureUtil::download(tex, img))
            return QString();

        img.save(QDir(sFolder).filePath(fileName).toStdString(), 0, 1);
    }
    catch (const std::exception& e)
    {
        qDebug() << "Failed to write image:" << fileName << e.what();
        return QString();
    }
    return fileName;
}
