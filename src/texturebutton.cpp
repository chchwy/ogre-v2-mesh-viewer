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
#include "OgreTextureGpuManager.h"
#include "OgreTextureBox.h"
#include "OgreImage2.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreResourceGroupManager.h"

#include "textureutil.h"


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

    Ogre::TextureGpu* texture = datablock->getTexture(textureType);
    if (texture == nullptr)
    {
        clear();
        return;
    }

    Ogre::Image2 img;
    if (!TextureUtil::download(texture, img))
    {
        clear();
        return;
    }

    setInfoText(QString::fromStdString(texture->getNameStr()), img.getWidth(), img.getHeight());

    QImage qtImg = toQtImage(img);
    if (qtImg.isNull())
    {
        qtImg = QImage(64, 64, QImage::Format_RGBA8888_Premultiplied);
        QPainter painter(&qtImg);
        painter.fillRect(QRect(0, 0, 64, 64), Qt::white);
        painter.drawText(13, 30, "Preview");
        painter.drawText(6, 42, "Unavailable");
    }

    QPixmap pixmap = QPixmap::fromImage(qtImg.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    mButton->setIconSize(QSize(64, 64));
    mButton->setIcon(QIcon(pixmap));
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
        Ogre::TextureGpu* texture = loadTexture(fileName);
        if (texture)
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

void TextureButton::clear()
{
    mButton->setIcon(QIcon());
    mButton->setIconSize(QSize(1, 1));
    mButton->setText("No Texture");
}

QImage TextureButton::toQtImage(const Ogre::Image2& img)
{
    const Ogre::TextureBox box = img.getData(0);
    const int width = int(box.width);
    const int height = int(box.height);

    switch (img.getPixelFormat())
    {
    case Ogre::PFG_RGBA8_UNORM:
    case Ogre::PFG_RGBA8_UNORM_SRGB:
    {
        QImage qtImg(width, height, QImage::Format_RGBA8888);
        for (int y = 0; y < height; ++y)
            memcpy(qtImg.scanLine(y), box.at(0, y, 0), size_t(width) * 4u);
        return qtImg;
    }
    case Ogre::PFG_BGRA8_UNORM:
    case Ogre::PFG_BGRA8_UNORM_SRGB:
    case Ogre::PFG_BGRX8_UNORM:
    case Ogre::PFG_BGRX8_UNORM_SRGB:
    {
        const bool hasAlpha = (img.getPixelFormat() == Ogre::PFG_BGRA8_UNORM || img.getPixelFormat() == Ogre::PFG_BGRA8_UNORM_SRGB);
        QImage qtImg(width, height, hasAlpha ? QImage::Format_ARGB32 : QImage::Format_RGB32);
        for (int y = 0; y < height; ++y)
            memcpy(qtImg.scanLine(y), box.at(0, y, 0), size_t(width) * 4u);
        return qtImg;
    }
    case Ogre::PFG_R8_UNORM:
    case Ogre::PFG_A8_UNORM:
    {
        QImage qtImg(width, height, QImage::Format_Grayscale8);
        for (int y = 0; y < height; ++y)
            memcpy(qtImg.scanLine(y), box.at(0, y, 0), size_t(width));
        return qtImg;
    }
    case Ogre::PFG_RG8_SNORM:
    case Ogre::PFG_RG8_UNORM:
    {
        // Two channel normal map: show it in the familiar blue-ish look
        const bool isSigned = (img.getPixelFormat() == Ogre::PFG_RG8_SNORM);
        QImage qtImg(width, height, QImage::Format_RGB32);
        for (int y = 0; y < height; ++y)
        {
            const uchar* src = static_cast<const uchar*>(box.at(0, y, 0));
            QRgb* dst = reinterpret_cast<QRgb*>(qtImg.scanLine(y));
            for (int x = 0; x < width; ++x)
            {
                int r = src[x * 2 + 0];
                int g = src[x * 2 + 1];
                if (isSigned)
                {
                    r = static_cast<signed char>(r) + 128;
                    g = static_cast<signed char>(g) + 128;
                }
                dst[x] = qRgb(r, g, 255);
            }
        }
        return qtImg;
    }
    default:
        qDebug() << "No preview for texture format:" << int(img.getPixelFormat());
        break;
    }
    return QImage();
}

Ogre::TextureGpu* TextureButton::loadTexture(const QString& texturePath)
{
    const QFileInfo info(texturePath);
    if (!info.exists() || info.suffix().isEmpty()) { return nullptr; }

    Ogre::CommonTextureTypes::CommonTextureTypes texType = Ogre::CommonTextureTypes::NonColourData;
    switch (mTextureType)
    {
    case Ogre::PBSM_DIFFUSE:
    case Ogre::PBSM_EMISSIVE:
        texType = Ogre::CommonTextureTypes::Diffuse;
        break;
    case Ogre::PBSM_METALLIC:
    case Ogre::PBSM_ROUGHNESS:
        texType = Ogre::CommonTextureTypes::Monochrome;
        break;
    case Ogre::PBSM_NORMAL:
        texType = Ogre::CommonTextureTypes::NormalMap;
        break;
    case Ogre::PBSM_REFLECTION:
        texType = Ogre::CommonTextureTypes::EnvMap;
        break;
    default:
        break;
    }

    const Ogre::String group = "ViewerResc";
    const Ogre::String folder = info.absolutePath().toStdString();
    const Ogre::String fileName = info.fileName().toStdString();

    try
    {
        auto& resourceMgr = Ogre::ResourceGroupManager::getSingleton();
        if (!resourceMgr.resourceLocationExists(folder, group))
            resourceMgr.addResourceLocation(folder, "FileSystem", group);

        // Decode once up front, so a broken file is reported instead of silently showing a dummy texture
        Ogre::Image2 probe;
        probe.load(fileName, group);
        if (probe.getWidth() == 0) { return nullptr; }

        // The same file may be wanted as e.g. both a colour and a monochrome texture
        const Ogre::String alias = texturePath.toStdString() + "#" + std::to_string(int(texType));
        Ogre::TextureGpu* texture = TextureUtil::manager()->createOrRetrieveTexture(
            fileName, alias, Ogre::GpuPageOutStrategy::Discard, texType, group);
        texture->scheduleTransitionTo(Ogre::GpuResidency::Resident);
        return texture;
    }
    catch (Ogre::Exception& e)
    {
        qDebug() << e.what();
    }
    return nullptr;
}
