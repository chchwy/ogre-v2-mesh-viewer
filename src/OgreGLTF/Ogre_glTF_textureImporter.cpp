#include "stdafx.h"

#include "Ogre_glTF_textureImporter.hpp"
#include "Ogre_glTF_common.hpp"
#include <OgreLogManager.h>
#include <OgreTextureGpuManager.h>
#include <OgreTextureGpu.h>
#include <OgreImage2.h>
#include <OgreColourValue.h>
#include <OgreRoot.h>
#include "Ogre_glTF.hpp"

using namespace Ogre_glTF;

//TODO pack the loaded textures in texture arrays (TEX_TYPE_2D_ARRAY) to optimize the way textures are binded to the slots of an HlmsPbsDatablock.
//TODO rethink the oder of operations while loading texture. Some of them need to be interpreted differently for they usage (MetalRoughMap needs to be separated in two greyscale map, NormalMap need SNORM reformating). Knowing what the material is doing with them will help avoid uncessesary resource usage and load time.
//TODO planned refactoring : pixel format selection code needs to be put into it's own method
//TODO planned refactoring : Loading of texture via OgreImage needs to be put into it's own method
//TODO investigate if HardwarePixelBuffer is going to be deprecated. Why is it in the Ogre::v1 namespace? What will happen in Ogre 2.2's "texture refactor"?

size_t textureImporter::id{ 0 };

void textureImporter::loadTexture(const tinygltf::Texture& texture)
{
    auto textureManager = Ogre::Root::getSingleton().getRenderSystem()->getTextureGpuManager();
    const auto& image = model.images[texture.source];
    const auto name = image.uri;

    auto OgreTexture = textureManager->findTextureNoThrow(Ogre::IdString(name));
    if (OgreTexture)
    {
        //OgreLog("Texture " + name + " already loaded in Ogre::TextureManager");
        return;
    }

    OgreLog("Loading texture image " + name);

    const auto pixelFormat = [&] {
        if (image.component == 3) return Ogre::PFG_RGBA8_UNORM;
        if (image.component == 4) return Ogre::PFG_RGB8_UNORM;

        throw InitError("Can get " + name + "pixel format");
    }();

    if (image.image.size() / image.component == image.width * image.height) { OgreLog("It looks like the image.component field and the image size does match"); }
    else
    {
        OgreLog("I have no idea what is going on with the image format");
    }

    Ogre::Image2* OgreImage = new Ogre::Image2;

    //The OgreImage class *can* take ownership of the pointer to the data and automatically delete it.
    //We *don't* want that. 6th argument needs to be set to false to prevent that.
    //The rest of the function is not modifying the model.images[x].image object. We get the image as a const ref.
    //In order to keep the rest of this code const correct, and knowing that the "autoDelete" is specifically
    //set to `false`, we're casting away const on the pointer to get the image data.
    OgreImage->loadDynamicImage(const_cast<Ogre::uchar*>(image.image.data()), image.width, image.height, 1, Ogre::TextureTypes::Type2D,  pixelFormat, false);

    OgreTexture = textureManager->createTexture(name, Ogre::GpuPageOutStrategy::Discard, Ogre::TextureFlags::ManualTexture, Ogre::TextureTypes::Type2D);
    OgreTexture->setPixelFormat(OgreImage->getPixelFormat());
    OgreTexture->setNumMipmaps(1);
    OgreTexture->setResolution(OgreImage->getWidth(), OgreImage->getHeight(), OgreImage->getNumSlices());
    OgreTexture->scheduleTransitionTo(Ogre::GpuResidency::Resident, OgreImage, true);
    //OgreTexture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
    //OgreTexture->_transitionTo()
    //OgreImage.uploadTo(OgreTexture, 0, 0);

    loadedTextures.insert({ texture.source, OgreTexture });
}

bool textureImporter::isHardwareGammaEnabled() const
{
    const auto renderSystem = Ogre::Root::getSingleton().getRenderSystem();

    //don't bother to check render targets for gamma if render system doesn't have the option enabled
    try
    {
        const auto renderSystemGammaConversionConfigOption = renderSystem->getConfigOptions().at("sRGB Gamma Conversion");
        if (renderSystemGammaConversionConfigOption.currentValue != "Yes") return false;
    }
    catch (const std::exception& e)
    {
        (void)e;
        OgreLog("It appears that render system doesn't know if it uses sRGB gamma conversion? How is that possible?");
    }

    return false;
}

textureImporter::textureImporter(tinygltf::Model& input) : model{ input } { id++; }

void textureImporter::loadTextures()
{
    for (const auto& texture : model.textures)
    {
        loadTexture(texture);
    }
}

Ogre::TextureGpu* textureImporter::getTexture(int glTFTextureSourceID)
{
    auto texture = loadedTextures.find(glTFTextureSourceID);
    if (texture == std::end(loadedTextures)) return {};

    return texture->second;
}

Ogre::TextureGpu* textureImporter::generateGreyScaleFromChannel(int gltfTextureSourceID, int channel)
{
    auto textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
    const auto& image = model.images[gltfTextureSourceID];

    assert(channel < 4 && channel >= 0 /*, "Channel needs to be between 0 and 3"*/);
    const auto name = "glTF_texture_" + image.name + std::to_string(id) + std::to_string(gltfTextureSourceID) + "_greyscale_channel" + std::to_string(channel);

    auto texture = textureManager->findTextureNoThrow(Ogre::IdString(name));
    if (texture)
    {
        OgreLog("texture " + name + "Already loaded in Ogre::TextureManager");
        return texture;
    }

    OgreLog("Can't find texture " + name + ". Generating it from glTF");

    assert(channel < image.component);

    //Grey-scale the image by putting all channel to the same value, ignoring alpha
    std::vector<Ogre::uchar> imageData(image.image.size());
    const auto pixelCount{ imageData.size() / image.component };
    for (size_t i{ 0 }; i < pixelCount; i++) //for each pixel
    {
        //Get the channel that has the value
        Ogre::uchar grey = image.image[(i * image.component) + channel];

        //Turn pixel at this specific shade of grey
        for (size_t c{ 0 }; c < 3; c++) imageData[i * image.component + c] = grey;

        //If there's an alpha channel, put it to 1.0f (255)
        if (image.component > 3) imageData[i * image.component + 3] = 255;
    }

    const auto pixelFormat = [&] {
        if (image.component == 3) return Ogre::PFG_RGB8_UNORM;
        if (image.component == 4) return Ogre::PFG_RGBA8_UNORM;

        OgreLog("unrecognized pixel format from tinygltf image");
        throw InitError("Can get " + name + "pixel format");
    }();

    //TODO refactor this

    //The OgreImage class *can* take ownership of the pointer to the data and automatically delete it.
    //We *don't* want that. 6th argument needs to be set to false to prevent that.
    //The rest of the function is not modifying the model.images[x].image object. We get the image as a const ref.
    //In order to keep the rest of this code const correct, and knowing that the "autoDelete" is specifically
    //set to `false`, we're casting away const on the pointer to get the image data.

    /*loadDynamicImage(void* pData, uint32 width, uint32 height, uint32 depthOrSlices,
     TextureTypes::TextureTypes textureType, PixelFormatGpu format,
     bool autoDelete, uint8 numMipmaps = 1u);*/
    Ogre::Image2 OgreImage;
    OgreImage.loadDynamicImage(imageData.data(), image.width, image.height, 1, Ogre::TextureTypes::Type2D, pixelFormat, false);

    Ogre::TextureGpu* OgreTexture = textureManager->createTexture(name, Ogre::GpuPageOutStrategy::Discard, 0, Ogre::TextureTypes::Type2D);
    OgreImage.uploadTo(OgreTexture, 1, 1);
    return OgreTexture;
}

Ogre::TextureGpu* textureImporter::getNormalSNORM(int gltfTextureSourceID)
{
    auto textureManager = Ogre::Root::getSingleton().getRenderSystem()->getTextureGpuManager();
    const auto& image = model.images[gltfTextureSourceID];
    const auto name = image.uri;

    auto texture = textureManager->findTextureNoThrow(Ogre::IdString(name));
    if (texture)
    {
        OgreLog("texture " + name + "Already loaded in Ogre::TextureManager");
        return texture;
    }

    OgreLog("Can't find texture " + name + ". Generating it from glTF");

    const auto pixelFormat = [&] {
        if (image.component == 3) return Ogre::PFG_RGB8_UNORM;
        if (image.component == 4) return Ogre::PFG_RGBA8_UNORM;

        OgreLog("unrecognized pixel format from tinygltf image");
        throw InitError("Can get " + name + "pixel format");
    }();

    const auto pixelFormatSnorm = [&] {
        if (image.component == 3) return Ogre::PFG_RGB8_UNORM;
        if (image.component == 4) return Ogre::PFG_RGBA8_SNORM;
        throw InitError("Can get " + name + "pixel format");
    }();

    Ogre::TextureGpu* OgreTexture = textureManager->createTexture(name, Ogre::GpuPageOutStrategy::Discard, 0, Ogre::TextureTypes::Type2D);
    /*
    auto pixels = OgreTexture->getBuffer()->lock({ 0, 0, unsigned(image.width), unsigned(image.height) }, //PixelBox that take the whole image
                                                 Ogre::v1::HardwareBuffer::LockOptions::HBL_NORMAL);
    //This loop convert BGR to RGB image data while also putting the value in the SNORM range [-1.0; +1.0]
    for (size_t y = 0; y < image.height; y++)
    {
        for (size_t x = 0; x < image.width; x++)
        {
            pixels.setColourAt(Ogre::ColourValue(2.0f * (float(image.image[image.component * (y * image.width + x) + 2]) / 255.0f) - 1.0f, //R to B
                               2.0f * (float(image.image[image.component * (y * image.width + x) + 1]) / 255.0f) - 1.0f, //G to G
                               2.0f * (float(image.image[image.component * (y * image.width + x) + 0]) / 255.0f) - 1.0f, //B to R
                               1.0f),
                               x,
                               y,
                               0);
        }
    }
    */
    //OgreTexture->getBuffer()->unlock();
    return OgreTexture;
}
