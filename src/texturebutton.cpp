#include "stdafx.h"
#include "texturebutton.h"

#include <QPushButton>
#include <QImage>

#include "OgreTexture.h"
#include "OgreImage.h"
#include "OgreHlmsPbsDatablock.h"


TextureButton::TextureButton(QPushButton* button) : QObject(button)
{
    Q_ASSERT(button);
    mButton = button;

    mButton->setStyleSheet("Text-align:left");
}

void TextureButton::setTexture(Ogre::HlmsPbsDatablock* datablock, Ogre::PbsTextureTypes textureType)
{
    Ogre::HlmsTextureManager::TextureLocation texLocation;
    texLocation.texture = datablock->getTexture(textureType);
    if (texLocation.texture.isNull())
    {
        clear();
        return;
    }

    Ogre::Image img;
    texLocation.texture->convertToImage(img);

    //qDebug() << "Ogre Format=" << img.getFormat();

    QImage::Format qtFormat = toQtImageFormat(img.getFormat());
    if (qtFormat != QImage::Format_Invalid)
    {
        QImage qImg(img.getData(), img.getWidth(), img.getHeight(), qtFormat);

        QPixmap pixmap = QPixmap::fromImage(qImg);
        mButton->setIconSize(QSize(64, 64));
        mButton->setIcon(QIcon(pixmap));
    }
    else
    {
        // TODO: show unknown format
        mButton->setIcon(QIcon());
        mButton->setIconSize(QSize(1, 1));
    }

    texLocation.xIdx = datablock->_getTextureIdx(textureType);
    texLocation.yIdx = 0;
    texLocation.divisor = 1;

    QString textureName;
    const Ogre::String* aliasName = getHlmsTexManager()->findAliasName(texLocation);
    if (aliasName)
    {
        textureName = QString::fromStdString(*aliasName);
    }

    setInfoText(textureName, img.getWidth(), img.getHeight());
}

void TextureButton::setInfoText(const QString& texName, int width, int height)
{
    QString texInfo = QString("%1\n%2x%3")
        .arg(texName)
        .arg(width)
        .arg(height);
    mButton->setText(texInfo);
}

Ogre::HlmsTextureManager* TextureButton::getHlmsTexManager()
{
    if (mHlmsTexManager == nullptr)
        mHlmsTexManager = Ogre::Root::getSingleton().getHlmsManager()->getTextureManager();
    return mHlmsTexManager;
}

void TextureButton::clear()
{
    mButton->setIcon(QIcon());
    mButton->setIconSize(QSize(1, 1));
    mButton->setText("No Texture");
}

QImage::Format TextureButton::toQtImageFormat(Ogre::PixelFormat ogreFormat)
{
    switch (ogreFormat)
    {
    case Ogre::PF_A8R8G8B8: return QImage::Format_ARGB32;
    case Ogre::PF_A8B8G8R8: return QImage::Format_Invalid;
    default:
        break;
    }
    return QImage::Format_Invalid;
}
