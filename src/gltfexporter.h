#pragma once

#include <vector>
#include <QString>

#include "meshdata.h"

/// Writes scene objects as glTF 2.0: one node per object, one primitive per submesh,
/// HlmsPbs datablocks as metallic-roughness materials with their textures embedded.
/// The file is binary (.glb) when the name ends in ".glb", otherwise JSON with embedded buffers.
class GltfExporter
{
public:
    bool writeToFile(const std::vector<MeshData::Object>& objects, const QString& sOutFile);
};
