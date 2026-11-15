#pragma once

#include "tiny_gltf.h"
#include <unordered_map>
#include <OgreTextureGpu.h>

namespace Ogre_glTF
{

///Import textures described in glTF into Ogre
class textureImporter
{
    ///How the pixels of a glTF image are interpreted when they become an Ogre texture
    enum class Usage
    {
        Colour,       ///< sRGB colour data (base colour, emissive)
        Normal,       ///< Tangent space normal map, stored as signed RG
        GreenChannel, ///< Greyscale made of the green channel (roughness)
        BlueChannel,  ///< Greyscale made of the blue channel (metalness)
    };

    ///Textures created so far, keyed by "image index, usage"
    std::unordered_map<int, Ogre::TextureGpu*> loadedTextures;

    ///Static counter to make unique texture name. Incremented by constructor
    static size_t id;
    size_t importerId;

    ///Reference to the tinygltf
    tinygltf::Model& model;

    ///Create (or fetch) the Ogre texture for a glTF texture index, converted for the given usage.
    ///Returns nullptr when the index is invalid or the image cannot be used.
    Ogre::TextureGpu* getOrCreate(int glTFTextureID, Usage usage);

public:
    ///Construct the texture importer object. Inrement the id counter
    /// \param input reference to the model that we are loading
    textureImporter(tinygltf::Model& input);

    ///Textures are created on demand by the getters below; kept for API compatibility
    void loadTextures();

    ///Get the colour texture that corespound to the given glTF texture index
    Ogre::TextureGpu* getTexture(int glTFTextureID);

    ///Get the texture that corespound to the given index, but as a greyscale one containing only
    ///the information of the given channel (0 = R, 1 = G, 2 = B)
    Ogre::TextureGpu* generateGreyScaleFromChannel(int glTFTextureID, int channel);

    ///Get the normal texture in a compatible format
    Ogre::TextureGpu* getNormalSNORM(int glTFTextureID);
};
}
