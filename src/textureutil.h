#pragma once

#include "OgrePrerequisites.h"
#include "OgreTextureGpu.h"

namespace TextureUtil
{
Ogre::TextureGpuManager* manager();

/// Creates a manual texture of the given type and uploads every mip of the image into it.
/// HlmsPbs samples its material textures as 2D arrays, so those need Type2DArray even for a
/// single 2D image; a plain Type2D texture would read as black.
/// If a texture with this name already exists, it is returned untouched.
Ogre::TextureGpu* createFromImage(const Ogre::String& name, Ogre::Image2& image,
                                  Ogre::TextureTypes::TextureTypes textureType);

/// Blocks until the texture is loaded, then downloads the selected mip into the image.
/// Returns false if the texture cannot be read back.
bool download(Ogre::TextureGpu* texture, Ogre::Image2& outImage, Ogre::uint8 mip = 0);
}
