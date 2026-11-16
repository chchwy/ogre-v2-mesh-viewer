#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "OgrePrerequisites.h"
#include "OgreMatrix4.h"

/// Plain CPU-side copies of Ogre v2 meshes, in the layout the exporters want:
/// uncompressed float positions, normals and UVs plus 32-bit triangle indices.
namespace MeshData
{
struct Vertex
{
    float position[3];
    float normal[3];
    float texcoord[2]; // Ogre convention, v = 0 at the top
    float tangent[4];  // xyz + handedness: bitangent = cross(normal, tangent) * w
};

struct SubMesh
{
    std::string materialName;
    Ogre::HlmsDatablock* datablock = nullptr; // only set when read from a scene item
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices; // triangle list
    bool hasNormals = false;
    bool hasTexcoords = false;
    bool hasTangents = false;
};

struct Object
{
    std::string name;
    Ogre::Matrix4 worldTransform = Ogre::Matrix4::IDENTITY;
    std::vector<SubMesh> subMeshes;
};

/// Reads every triangle-list submesh of the mesh. Other primitive types are skipped.
std::vector<SubMesh> readMesh(Ogre::Mesh* mesh);

/// Reads every Item attached below (and on) the node, with its world transform and datablocks.
std::vector<Object> readScene(Ogre::SceneNode* root);
}
