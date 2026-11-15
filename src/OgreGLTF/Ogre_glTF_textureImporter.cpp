#include "stdafx.h"

#include "Ogre_glTF_textureImporter.hpp"
#include "Ogre_glTF_common.hpp"
#include <OgreLogManager.h>
#include <OgreImage2.h>
#include <OgreTextureBox.h>
#include "Ogre_glTF.hpp"
#include "textureutil.h"

using namespace Ogre_glTF;

size_t textureImporter::id{ 0 };

textureImporter::textureImporter(tinygltf::Model& input) : model{ input } { importerId = id++; }

void textureImporter::loadTextures()
{
}

Ogre::TextureGpu* textureImporter::getOrCreate(int glTFTextureID, Usage usage)
{
    if (glTFTextureID < 0 || glTFTextureID >= int(model.textures.size())) return nullptr;

    const int imageIndex = model.textures[glTFTextureID].source;
    if (imageIndex < 0 || imageIndex >= int(model.images.size())) return nullptr;

    const int key = imageIndex * 4 + int(usage);
    const auto found = loadedTextures.find(key);
    if (found != loadedTextures.end()) return found->second;

    const tinygltf::Image& image = model.images[imageIndex];
    const std::string label = image.uri.empty() ? image.name : image.uri;

    if (image.bits != 8 || image.component != 4 || image.width <= 0 || image.height <= 0 ||
        image.image.size() < size_t(image.width) * size_t(image.height) * 4u)
    {
        OgreLog("Unsupported glTF image layout, skipping texture " + label);
        loadedTextures[key] = nullptr;
        return nullptr;
    }

    static const char* suffixes[] = { "", "_normal", "_green", "_blue" };
    const std::string name = "glTF_" + std::to_string(importerId) + "_" + std::to_string(imageIndex) + "_" + label + suffixes[int(usage)];

    const Ogre::uint32 width = Ogre::uint32(image.width);
    const Ogre::uint32 height = Ogre::uint32(image.height);

    Ogre::PixelFormatGpu format = Ogre::PFG_RGBA8_UNORM_SRGB;
    switch (usage)
    {
    case Usage::Colour: format = Ogre::PFG_RGBA8_UNORM_SRGB; break;
    case Usage::Normal: format = Ogre::PFG_RG8_SNORM; break;
    case Usage::GreenChannel:
    case Usage::BlueChannel: format = Ogre::PFG_R8_UNORM; break;
    }

    Ogre::Image2 ogreImage;
    ogreImage.createEmptyImage(width, height, 1u, Ogre::TextureTypes::Type2D, format);
    const Ogre::TextureBox box = ogreImage.getData(0);

    //tinygltf always hands us tightly packed 8 bits RGBA
    const unsigned char* src = image.image.data();
    for (Ogre::uint32 y = 0; y < height; ++y)
    {
        const unsigned char* srcRow = src + size_t(y) * width * 4u;
        void* dstRow = box.at(0, y, 0);

        switch (usage)
        {
        case Usage::Colour: memcpy(dstRow, srcRow, size_t(width) * 4u); break;
        case Usage::Normal:
        {
            Ogre::int8* dst = static_cast<Ogre::int8*>(dstRow);
            for (Ogre::uint32 x = 0; x < width; ++x)
            {
                dst[x * 2 + 0] = Ogre::int8(int(srcRow[x * 4 + 0]) - 128);
                dst[x * 2 + 1] = Ogre::int8(int(srcRow[x * 4 + 1]) - 128);
            }
            break;
        }
        case Usage::GreenChannel:
        case Usage::BlueChannel:
        {
            const int channel = (usage == Usage::GreenChannel) ? 1 : 2;
            Ogre::uint8* dst = static_cast<Ogre::uint8*>(dstRow);
            for (Ogre::uint32 x = 0; x < width; ++x) dst[x] = srcRow[x * 4 + channel];
            break;
        }
        }
    }

    ogreImage.generateMipmaps(usage == Usage::Colour);

    OgreLog("Creating texture " + name);
    Ogre::TextureGpu* texture = TextureUtil::createFromImage(name, ogreImage, Ogre::TextureTypes::Type2DArray);
    loadedTextures[key] = texture;
    return texture;
}

Ogre::TextureGpu* textureImporter::getTexture(int glTFTextureID)
{
    return getOrCreate(glTFTextureID, Usage::Colour);
}

Ogre::TextureGpu* textureImporter::generateGreyScaleFromChannel(int glTFTextureID, int channel)
{
    assert(channel == 1 || channel == 2);
    return getOrCreate(glTFTextureID, channel == 1 ? Usage::GreenChannel : Usage::BlueChannel);
}

Ogre::TextureGpu* textureImporter::getNormalSNORM(int glTFTextureID)
{
    return getOrCreate(glTFTextureID, Usage::Normal);
}
