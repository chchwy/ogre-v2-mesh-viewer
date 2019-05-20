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
    TextureButton(QPushButton*);

    void setTexture(Ogre::HlmsPbsDatablock*, Ogre::PbsTextureTypes);
    void clear();

    static QImage::Format toQtImageFormat(Ogre::PixelFormat);

private:
    void setInfoText(const QString& texName, int width, int height);
    Ogre::HlmsTextureManager* getHlmsTexManager();

    Ogre::HlmsTextureManager* mHlmsTexManager = nullptr;
    QPushButton* mButton = nullptr;
};

#endif // TEXTUREBUTTON_H