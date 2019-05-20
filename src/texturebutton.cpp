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
    texLocation.xIdx = datablock->_getTextureIdx(textureType);
    texLocation.yIdx = 0;
    texLocation.divisor = 1;

    if (texLocation.texture.isNull())
    {
        clear();
        return;
    }

    //qDebug() << "Type:" << (int)textureType
    //         << ", X index:" << texLocation.xIdx
    //         << ", Pointer:" << texLocation.texture;

    Ogre::Image img;
    texLocation.texture->convertToImage(img, false, 0, texLocation.xIdx, 1);
    
    int originalWidth = img.getWidth();
    int originalHeight = img.getHeight();
    //img.resize(64, 64);

    QString textureName;
    const Ogre::String* aliasName = getHlmsTexManager()->findAliasName(texLocation);
    if (aliasName)
    {
        textureName = QString::fromStdString(*aliasName);
    }
    setInfoText(textureName, originalWidth, originalHeight);
    
    QImage qtImg = toQtImage(img);
    if (!qtImg.isNull())
    {
        QPixmap pixmap = QPixmap::fromImage(qtImg);
        mButton->setIconSize(QSize(64, 64));
        mButton->setIcon(QIcon(pixmap));
    }
    else
    {
        // TODO: show unknown format
        mButton->setIcon(QIcon());
        mButton->setIconSize(QSize(1, 1));
    }
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
    case Ogre::PF_A8: return QImage::Format_Alpha8;
    case Ogre::PF_L8: return QImage::Format_Grayscale8;
    //case Ogre::PF_A8B8G8R8: return QImage::Format_Invalid;
    default:
        qDebug() << "Unknown tex format:" << ogreFormat;
        break;
    }
    return QImage::Format_Invalid;
}

QImage TextureButton::toQtImage(const Ogre::Image& img)
{
    QImage::Format qtFormat = toQtImageFormat(img.getFormat());
    if (qtFormat != QImage::Format_Invalid)
    {
        QImage qImg(img.getData(), img.getWidth(), img.getHeight(), qtFormat);
        return qImg;
    }

    switch(img.getFormat())
    {
    case Ogre::PF_R8G8_SNORM:
    {
        QImage qtImg(img.getWidth(), img.getHeight(), QImage::Format_RGBA8888_Premultiplied);

        char* src = (char*)img.getData();
        uchar* dest = qtImg.bits();

        for (int x = 0; x < img.getWidth(); ++x)
        {
            for (int y = 0; y < img.getHeight(); ++y)
            {
                size_t src_byte_offset = x * y * 2;
                int r = src[src_byte_offset + 0] + 128;
                int g = src[src_byte_offset + 1] + 128;

                qtImg.setPixel(x, y, qRgb(r, g, 255));
                //size_t src_byte_offset = x * y * 2;
                //qDebug() << (int)src[src_byte_offset + 0] << (int)src[src_byte_offset + 1];
            }
        }
        qtImg.save("C:/Temp/normal.png");
        return qtImg;
        break;
    }
    default:
        break;
    }
    return QImage();
}
