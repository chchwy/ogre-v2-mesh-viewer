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

#include "OgreTextureGpu.h"
#include "OgreImage2.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgrePixelFormatGpu.h"
#include "OgreTextureBox.h"


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

    Ogre::TextureGpu* texture = nullptr;
    texture = datablock->getTexture(textureType);
    
    if (texture)
    {
        clear();
        return;
    }

    //qDebug() << "Type:" << (int)textureType
    //         << ", X index:" << texLocation.xIdx
    //         << ", Pointer:" << texLocation.texture;

    Ogre::Image2 img;
    img.convertFromTexture(texture, 0, 0);
    
    int originalWidth = img.getWidth();
    int originalHeight = img.getHeight();
    //img.resize(64, 64);

    QString textureName;
    const Ogre::String* aliasName = getHlmsTexManager()->findAliasNameStr(texture->getName());
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
        Ogre::TextureGpu* texture = nullptr;
        bool ok = LoadImage(fileName, texture);
        if (ok)
        {
            mDatablock->setTexture(mTextureType, texture);
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

Ogre::TextureGpuManager* TextureButton::getHlmsTexManager()
{
    if (mHlmsTexManager == nullptr)
        mHlmsTexManager = Ogre::Root::getSingleton().getRenderSystem()->getTextureGpuManager();
    return mHlmsTexManager;
}

void TextureButton::clear()
{
    mButton->setIcon(QIcon());
    mButton->setIconSize(QSize(1, 1));
    mButton->setText("No Texture");
}

QImage::Format TextureButton::toQtImageFormat(Ogre::PixelFormatGpu ogreFormat)
{
    switch (ogreFormat)
    {
    case Ogre::PFG_RGBA32_FLOAT: return QImage::Format_ARGB32;
    //case Ogre::PFG_A8: return QImage::Format_Alpha8;
    //case Ogre::PFG_L8: return QImage::Format_Grayscale8;
    //case Ogre::PF_A8B8G8R8: return QImage::Format_Invalid;
    default:
        qDebug() << "Unknown tex format:" << ogreFormat;
        break;
    }
    return QImage::Format_Invalid;
}

QImage TextureButton::toQtImage(const Ogre::Image2& img)
{
    QImage::Format qtFormat = toQtImageFormat(img.getPixelFormat());
    if (qtFormat != QImage::Format_Invalid)
    {
        QImage qImg((uchar*)img.getData(0).data, img.getWidth(), img.getHeight(), qtFormat);
        return qImg;
    }

    switch(img.getPixelFormat())
    {
        /*
    case Ogre::PFG_R8G8_SNORM:
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
    case Ogre::PFG_A8B8G8R8:
    {
        QImage qtImg(img.getWidth(), img.getHeight(), QImage::Format_RGBA8888_Premultiplied);
        break;
    }*/
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

bool TextureButton::LoadImage(const QString& texturePath, Ogre::TextureGpu*& texture)
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
        Ogre::Image2 img;
        img.load(dataStream, fileExt.toStdString());

        if (img.getWidth() == 0) { return false; }

        switch (mTextureType) {
            break;
        }
        texture = getHlmsTexManager()->createTexture(texName, texName,
            Ogre::GpuPageOutStrategy::Discard,
            Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB,
            Ogre::TextureTypes::Type2D);
        ok = true;
    }
    catch(Ogre::Exception& e)
    {
        qDebug() << e.what();
        ok = false;
    }
    return ok;
}