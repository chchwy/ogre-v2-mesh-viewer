#include "materialwidget.h"
#include "ui_materialwidget.h"

#include "OgreHlmsPbsDatablock.h"
#include "spinslider.h"

MaterialWidget::MaterialWidget(QWidget* parent) : QWidget(parent)
{
    ui = new Ui::MaterialWidget;
    ui->setupUi(this);

    ui->diffuseTexButton->setStyleSheet("Text-align:left");
    ui->diffuseColorButton->setStyleSheet("Text-align:left");
    ui->diffuseBgColorButton->setStyleSheet("Text-align:left");

    auto comboSignal = static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged);
    connect(ui->mtlNameCombo, comboSignal, this, &MaterialWidget::materialComboIndexChanged);
    connect(ui->wireframeCheck, &QCheckBox::clicked, this, &MaterialWidget::wireFrameClicked);

    mTransparencySpinSlider = new SpinSlider(ui->transparencySlider, ui->transparencySpin);
    connect(mTransparencySpinSlider, &SpinSlider::valueChanged, this, &MaterialWidget::transparencyValueChanged);

    connect(ui->transModeRadioTrans, &QRadioButton::clicked, this, &MaterialWidget::transparencyModeChanged);
    connect(ui->transModeRadioFade, &QRadioButton::clicked, this, &MaterialWidget::transparencyModeChanged);
    connect(ui->transModeRadioNone, &QRadioButton::clicked, this, &MaterialWidget::transparencyModeChanged);
    connect(ui->useTextureAlphaCheck, &QCheckBox::clicked, this, &MaterialWidget::useAlphaFromTextureClicked);
}

MaterialWidget::~MaterialWidget()
{
    delete ui;
}

void MaterialWidget::sceneNodeSelected(Ogre::SceneNode* node)
{
    Ogre::Item* item = getFirstItem(node);
    mCurrentItem = item;

    if (mCurrentItem)
    {
        enableAll();
        updateMaterialCombo();
        updateOneDatablock();
    }
    else
    {
        disableAll();
    }
}

void MaterialWidget::materialComboIndexChanged(int i)
{
    if (i >= 0)
    {
        updateOneDatablock();
    }
}

void MaterialWidget::wireFrameClicked(bool b)
{
    Ogre::HlmsPbsDatablock* pbs = getCurrentDatablock();
    if (pbs)
    {
        Ogre::HlmsMacroblock macro = *pbs->getMacroblock();
        macro.mPolygonMode = (b) ? Ogre::PM_WIREFRAME : Ogre::PM_SOLID;
        pbs->setMacroblock(macro);
    }
}

void MaterialWidget::transparencyValueChanged(double value)
{
    Ogre::HlmsPbsDatablock* pbs = getCurrentDatablock();
    auto currentMode = pbs->getTransparencyMode();
    pbs->setTransparency(value, currentMode);
}

void MaterialWidget::transparencyModeChanged()
{
    Ogre::HlmsPbsDatablock* pbs = getCurrentDatablock();
    float currentAlpha = pbs->getTransparency();

    if (ui->transModeRadioTrans->isChecked())
    {
        pbs->setTransparency(currentAlpha, Ogre::HlmsPbsDatablock::Transparent);
    }
    else if (ui->transModeRadioFade->isChecked())
    {
        pbs->setTransparency(currentAlpha, Ogre::HlmsPbsDatablock::Fade);
    }
    else
    {
        pbs->setTransparency(currentAlpha, Ogre::HlmsPbsDatablock::None);
    }
}

void MaterialWidget::useAlphaFromTextureClicked(bool b)
{
    Ogre::HlmsPbsDatablock* pbs = getCurrentDatablock();
    pbs->setTransparency(pbs->getTransparency(), pbs->getTransparencyMode(), b);
}

Ogre::Item* MaterialWidget::getFirstItem(Ogre::SceneNode* node)
{
    if (node)
    {
        size_t num = node->numAttachedObjects();
        for (int i = 0; i < num; ++i)
        {
            Ogre::Item* item = dynamic_cast<Ogre::Item*>(node->getAttachedObject(i));
            if (item)
                return item;
        }
    }
    return nullptr;
}

Ogre::HlmsPbsDatablock* MaterialWidget::getCurrentDatablock()
{
    int i = ui->mtlNameCombo->currentIndex();
    
    if (i >= 0)
    {
        Ogre::HlmsDatablock* datablock = mCurrentItem->getSubItem(i)->getDatablock();
        Ogre::HlmsPbsDatablock* pbsblock = dynamic_cast<Ogre::HlmsPbsDatablock*>(datablock);
        if (nullptr != pbsblock)
        {
            return pbsblock;
        }
    }
    return nullptr;
}

void MaterialWidget::updateMaterialCombo()
{
    Q_ASSERT(mCurrentItem);

    ui->mtlNameCombo->clear();

    int num = mCurrentItem->getNumSubItems();
    for (int i = 0; i < num; ++i)
    {
        Ogre::HlmsDatablock* datablock = mCurrentItem->getSubItem(i)->getDatablock();

        QString name = QString::fromStdString(*datablock->getNameStr());
        QString displayText = QString("%1: %2").arg(i).arg(name);
        ui->mtlNameCombo->addItem(displayText, QVariant(i));
    }
}

void MaterialWidget::updateOneDatablock()
{
    Ogre::HlmsPbsDatablock* pbs = getCurrentDatablock();
    Ogre::PolygonMode polygonMode = pbs->getMacroblock()->mPolygonMode;
    
    QSignalBlocker(ui->wireframeCheck);
    ui->wireframeCheck->setChecked(polygonMode == Ogre::PM_WIREFRAME);

    updateDiffuseGroup(pbs);
    updateTransparencyGroup(pbs);
}

void MaterialWidget::updateDiffuseGroup(Ogre::HlmsPbsDatablock* pbs)
{
    Q_ASSERT(mCurrentItem);
    
    Ogre::TexturePtr tex = pbs->getTexture(Ogre::PBSM_DIFFUSE);

    if (tex)
    {
        Ogre::Image img;
        tex->convertToImage(img);

        qDebug() << "Ogre Format=" << img.getFormat();

        QImage::Format qtFormat = toQtImageFormat(img.getFormat());
        if (qtFormat != QImage::Format_Invalid)
        {
            QImage qImg(img.getData(), img.getWidth(), img.getHeight(), qtFormat);

            QPixmap pixmap = QPixmap::fromImage(qImg);
            ui->diffuseTexButton->setIconSize(QSize(64, 64));
            ui->diffuseTexButton->setIcon(QIcon(pixmap));
        }
        else
        {
            // TODO: show unknown format
        }

        QString texInfo = QString("%1: %2x%3")
            .arg(tex->getName().c_str())
            .arg(img.getWidth())
            .arg(img.getHeight());
        ui->diffuseTexButton->setText(texInfo);
    }
    else
    {
        ui->diffuseTexButton->setIcon(QIcon());
        ui->diffuseTexButton->setIconSize(QSize(1, 1));
        ui->diffuseTexButton->setText("No Texture");
    }

    Ogre::Vector3 diffuse = pbs->getDiffuse() * 255;
    QPixmap diffPixmap(64, 16);
    diffPixmap.fill(QColor(diffuse.x, diffuse.y, diffuse.z));
    ui->diffuseColorButton->setIcon(diffPixmap);
    ui->diffuseColorButton->setIconSize(QSize(64, 16));

    Ogre::ColourValue bgDiffuse = pbs->getBackgroundDiffuse() * 255;
    QPixmap bgDiffPixmap(64, 16);
    bgDiffPixmap.fill(QColor(bgDiffuse.r, bgDiffuse.g, bgDiffuse.b, bgDiffuse.a));
    ui->diffuseBgColorButton->setIcon(bgDiffPixmap);
    ui->diffuseBgColorButton->setIconSize(QSize(64, 16));
}

void MaterialWidget::updateTransparencyGroup(Ogre::HlmsPbsDatablock* pbs)
{
    Q_ASSERT(mCurrentItem);

    float alphaValue = pbs->getTransparency();

    QSignalBlocker b1(mTransparencySpinSlider);
    mTransparencySpinSlider->setValue(alphaValue);

    QSignalBlocker b3(ui->transModeRadioTrans);
    QSignalBlocker b4(ui->transModeRadioFade);
    QSignalBlocker b5(ui->transModeRadioNone);

    Ogre::HlmsPbsDatablock::TransparencyModes mode = pbs->getTransparencyMode();
    switch (mode)
    {
    case Ogre::HlmsPbsDatablock::Transparent: ui->transModeRadioTrans->setChecked(true); break;
    case Ogre::HlmsPbsDatablock::Fade: ui->transModeRadioFade->setChecked(true); break;
    case Ogre::HlmsPbsDatablock::None: ui->transModeRadioNone->setChecked(true); break;
    }

    QSignalBlocker b7(ui->useTextureAlphaCheck);
    ui->useTextureAlphaCheck->setChecked(pbs->getUseAlphaFromTextures());
}

void MaterialWidget::enableAll()
{
    ui->diffuseGroup->setEnabled(true);
}

void MaterialWidget::disableAll()
{
    ui->diffuseGroup->setEnabled(false);
}

QImage::Format MaterialWidget::toQtImageFormat(Ogre::PixelFormat ogreFormat)
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