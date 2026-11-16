#ifndef OBJEXPORTER_H
#define OBJEXPORTER_H

#include <string>
#include <vector>
#include <QString>

#include "meshdata.h"

namespace Ogre
{
    class TextureGpu;
}

/// Writes scene objects as one Wavefront .obj (world transforms baked in) plus its .mtl.
class ObjExporter
{
public:
    bool writeToFile(const std::vector<MeshData::Object>& objects, const QString& sOutFile);

private:
    bool writeObjFile(const std::vector<MeshData::Object>& objects, const QString& sOutObjFile, const QString& sMtlFileName);
    bool writeMtlFile(const std::vector<MeshData::Object>& objects, const QString& sOutFile);

    /// Saves the texture next to the .mtl. Returns the file name, or an empty string on failure.
    QString writeTexture(Ogre::TextureGpu* tex, const QString& sFolder, const QString& sBaseName);

    static QString materialName(const MeshData::SubMesh& subMesh);
};

#endif // OBJEXPORTER_H
