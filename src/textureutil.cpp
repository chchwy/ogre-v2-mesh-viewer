#include "stdafx.h"
#include "textureutil.h"

#include "OgreRoot.h"
#include "OgreRenderSystem.h"
#include "OgreTextureGpuManager.h"
#include "OgreImage2.h"


namespace TextureUtil
{

Ogre::TextureGpuManager* manager()
{
    return Ogre::Root::getSingleton().getRenderSystem()->getTextureGpuManager();
}

Ogre::TextureGpu* createFromImage(const Ogre::String& name, Ogre::Image2& image,
                                  Ogre::TextureTypes::TextureTypes textureType)
{
    Ogre::TextureGpuManager* texMgr = manager();

    Ogre::TextureGpu* existing = texMgr->findTextureNoThrow(name);
    if (existing)
        return existing;

    Ogre::TextureGpu* texture = texMgr->createTexture(name,
                                                      Ogre::GpuPageOutStrategy::Discard,
                                                      Ogre::TextureFlags::ManualTexture,
                                                      textureType);
    texture->setResolution(image.getWidth(), image.getHeight(), image.getDepthOrSlices());
    texture->setPixelFormat(image.getPixelFormat());
    texture->setNumMipmaps(image.getNumMipmaps());

    texture->_transitionTo(Ogre::GpuResidency::Resident, nullptr);
    texture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
    image.uploadTo(texture, 0, texture->getNumMipmaps() - 1u);

    return texture;
}

bool download(Ogre::TextureGpu* texture, Ogre::Image2& outImage, Ogre::uint8 mip)
{
    try
    {
        texture->waitForData();
        mip = std::min<Ogre::uint8>(mip, texture->getNumMipmaps() - 1u);
        outImage.convertFromTexture(texture, mip, mip);
    }
    catch (Ogre::Exception& e)
    {
        qDebug() << "Cannot read texture back:" << e.what();
        return false;
    }
    return true;
}

}
