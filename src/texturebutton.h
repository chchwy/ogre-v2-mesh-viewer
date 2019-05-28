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

    static QImage::Format toQtImageFormat(Ogre::PixelFormat);
    static QImage toQtImage(const Ogre::Image& img);

private:
    void buttonClicked();

    void setInfoText(const QString& texName, int width, int height);
    Ogre::HlmsTextureManager* getHlmsTexManager();
    bool LoadImage(const Ogre::String& texturePath, Ogre::HlmsTextureManager::TextureLocation& loc);

    Ogre::PbsTextureTypes mTextureType = Ogre::PBSM_DIFFUSE;
    Ogre::HlmsPbsDatablock* mDatablock = nullptr;
    Ogre::HlmsTextureManager* mHlmsTexManager = nullptr;
    QPushButton* mButton = nullptr;
};

#endif // TEXTUREBUTTON_H