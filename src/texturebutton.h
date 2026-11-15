#ifndef TEXTUREBUTTON_H
#define TEXTUREBUTTON_H

#include <QObject>
#include <QImage>
#include "OgreHlmsPbsPrerequisites.h"

class QPushButton;
namespace Ogre
{
class HlmsPbsDatablock;
class Image2;
class TextureGpu;
}

class TextureButton : public QObject
{
    Q_OBJECT

public:
    TextureButton(QPushButton*, Ogre::PbsTextureTypes);

    void updateTexImage(Ogre::HlmsPbsDatablock*, Ogre::PbsTextureTypes);
    void clear();

    /// Converts mip 0, slice 0 of the image. Returns a null QImage for unsupported formats.
    static QImage toQtImage(const Ogre::Image2& img);

private:
    void buttonClicked();

    void setInfoText(const QString& texName, int width, int height);
    Ogre::TextureGpu* loadTexture(const QString& texturePath);

    Ogre::PbsTextureTypes mTextureType = Ogre::PBSM_DIFFUSE;
    Ogre::HlmsPbsDatablock* mDatablock = nullptr;
    QPushButton* mButton = nullptr;
};

#endif // TEXTUREBUTTON_H
