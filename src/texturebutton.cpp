#include "stdafx.h"
#include "texturebutton.h"

#include <QPushButton>
#include <QImage>
#include <QFileDialog>
#include <QSettings>
#include <QStandardPaths>
#include <QPainter>
#include <QFileInfo>
#include <QMessageBox>

#include "OgreTexture.h"
#include "OgreImage.h"
#include "OgreHlmsPbsDatablock.h"


TextureButton::TextureButton(QPushButton* button, Ogre::PbsTextureTypes texType) : QObject(button)
{
    Q_ASSERT(button);
    mButton = button;
    //mButton->setStyleSheet("Text-align:left");

    mTextureType = texType;

    connect(button, &QPushButton::clicked, this, &TextureButton::buttonClicked);
}

void TextureButton::updateTexImage(Ogre::HlmsPbsDatablock* datablock, Ogre::PbsTextureTypes textureType)
{
    mDatablock = datablock;

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

void TextureButton::buttonClicked()
{
    QSettings settings("OgreV2ModelViewer", "OgreV2ModelViewer");
    QString myDocument = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0];
    QString defaultFolder = settings.value("textureLocation", myDocument).toString();

    QString fileName = QFileDialog::getOpenFileName(mButton, "Load textures", defaultFolder, "Images (*.png *.jpg *.dds *.bmp *.tga *.hdr)");
    if (fileName.isEmpty()) { return; }

    if (mDatablock)
    {
        Ogre::HlmsTextureManager::TextureLocation loc;
        bool ok = LoadImage(fileName, loc);
        if (ok)
        {
            mDatablock->setTexture(mTextureType, loc.xIdx, loc.texture);
            updateTexImage(mDatablock, mTextureType);

            settings.setValue("textureLocation", QFileInfo(fileName).absolutePath());
        }
        else
        {
            QMessageBox::information(mButton, "Error", "Ogre3D cannot load this texture");
        }
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
        for (size_t x = 0; x < img.getWidth(); ++x)
        {
            for (size_t y = 0; y < img.getHeight(); ++y)
            {
                size_t src_byte_offset = x * y * 2;
                int r = src[src_byte_offset + 0] + 128;
                int g = src[src_byte_offset + 1] + 128;
                qtImg.setPixel((int)x, (int)y, qRgb(r, g, 255));
            }
        }
        //qtImg.save("C:/Temp/normal.png");
        return qtImg;
        break;
    }
    case Ogre::PF_A8B8G8R8:
    {
        QImage qtImg(img.getWidth(), img.getHeight(), QImage::Format_RGBA8888_Premultiplied);
        break;
    }
    default:
        break;
    }

    static QImage emptyImg(64, 64, QImage::Format_RGBA8888_Premultiplied);
    QPainter painter(&emptyImg);
    painter.fillRect(QRect(0, 0, 64, 64), Qt::white);
    painter.drawText(13, 30, "Preview");
    painter.drawText(6, 42, "Unavailable");
    return emptyImg;
}

bool TextureButton::LoadImage(const QString& texturePath, Ogre::HlmsTextureManager::TextureLocation& loc)
{
    bool ok = false;
    std::ifstream ifs(texturePath.toStdWString(), std::ios::binary | std::ios::in);
    ON_SCOPE_EXIT(ifs.close());
    if (!ifs.is_open()) { return false; }

    QString fileExt = QFileInfo(texturePath).suffix();
    if (fileExt.isEmpty()) { return false; }


    std::string texName = texturePath.toStdString();
    Ogre::DataStreamPtr dataStream(new Ogre::FileStreamDataStream(texName, &ifs, false));

    try
    {
        Ogre::Image img;
        img.load(dataStream, fileExt.toStdString());

        if (img.getWidth() == 0) { return false; }

        Ogre::HlmsTextureManager::TextureMapType mapType;
        switch (mTextureType) {
        case Ogre::PBSM_DIFFUSE:
        case Ogre::PBSM_EMISSIVE:
            mapType = Ogre::HlmsTextureManager::TEXTURE_TYPE_DIFFUSE;
            break;
        case Ogre::PBSM_METALLIC:
        case Ogre::PBSM_ROUGHNESS:
            mapType = Ogre::HlmsTextureManager::TEXTURE_TYPE_MONOCHROME;
            break;
        case Ogre::PBSM_NORMAL:
            mapType = Ogre::HlmsTextureManager::TEXTURE_TYPE_NORMALS;
            break;
        case Ogre::PBSM_REFLECTION:
            mapType = Ogre::HlmsTextureManager::TEXTURE_TYPE_ENV_MAP;
            break;
        default:
            Ogre::HlmsTextureManager::TEXTURE_TYPE_DETAIL;
            Ogre::HlmsTextureManager::TEXTURE_TYPE_DETAIL_NORMAL_MAP;
            Ogre::HlmsTextureManager::TEXTURE_TYPE_NON_COLOR_DATA;
            break;
        }
        loc = getHlmsTexManager()->createOrRetrieveTexture(texName, texName, mapType, 0, &img);
        ok = true;
    }
    catch(Ogre::Exception& e)
    {
        qDebug() << e.what();
        ok = false;
    }
    return ok;
}