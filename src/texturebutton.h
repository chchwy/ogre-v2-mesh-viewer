#ifndef TEXTUREBUTTON_H
#define TEXTUREBUTTON_H

#include <QObject>
#include "OgreHlmsPbsPrerequisites.h"

class QPushButton;
namespace Ogre
{
class HlmsTextureManager;
class HlmsPbsDatablock;
}

class TextureButton : public QObject
{
    Q_OBJECT

public:
    TextureButton(QPushButton*, Ogre::PbsTextureTypes);

    void updateTexImage(Ogre::HlmsPbsDatablock*, Ogre::PbsTextureTypes);
    void clear();

    static QImage::Format toQtImageFormat(Ogre::PixelFormatGpu);
    static QImage toQtImage(const Ogre::Image2& img);

private:
    void buttonClicked();

    void setInfoText(const QString& texName, int width, int height);
    Ogre::TextureGpuManager* getHlmsTexManager();
    bool LoadImage(const QString& texturePath, Ogre::TextureGpu*& loc);

    Ogre::PbsTextureTypes mTextureType = Ogre::PBSM_DIFFUSE;
    Ogre::HlmsPbsDatablock* mDatablock = nullptr;
    Ogre::TextureGpuManager* mHlmsTexManager = nullptr;
    QPushButton* mButton = nullptr;
};

#endif // TEXTUREBUTTON_H