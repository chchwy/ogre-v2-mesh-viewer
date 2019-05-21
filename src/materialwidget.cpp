#include "stdafx.h"
#include "materialwidget.h"
#include "ui_materialwidget.h"

#include <QColorDialog>
#include "OgreHlmsPbsDatablock.h"

#include "spinslider.h"
#include "texturebutton.h"


MaterialWidget::MaterialWidget(QWidget* parent) : QWidget(parent)
{
    ui = new Ui::MaterialWidget;
    ui->setupUi(this);

    /// General section
    auto comboSignal = static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged);
    connect(ui->mtlNameCombo, comboSignal, this, &MaterialWidget::materialComboIndexChanged);
    connect(ui->wireframeCheck, &QCheckBox::clicked, this, &MaterialWidget::wireFrameClicked);
    connect(ui->twoSidedCheck, &QCheckBox::clicked, this, &MaterialWidget::twoSidedClicked);

    /// Diffuse section
    mDiffuseTexButton = new TextureButton(ui->diffuseTexButton);
    ui->diffuseColorButton->setStyleSheet("Text-align:left");
    ui->diffuseBgColorButton->setStyleSheet("Text-align:left");

    connect(ui->diffuseColorButton, &QPushButton::clicked, this, &MaterialWidget::diffuseColorButtonClicked);
    connect(ui->diffuseBgColorButton, &QPushButton::clicked, this, &MaterialWidget::diffuseBgColorButtonClicked);

    /// Transparency section
    mTransparencySpinSlider = new SpinSlider(ui->transparencySlider, ui->transparencySpin);
    connect(mTransparencySpinSlider, &SpinSlider::valueChanged, this, &MaterialWidget::transparencyValueChanged);

    connect(ui->transModeRadioTrans, &QRadioButton::clicked, this, &MaterialWidget::transparencyModeChanged);
    connect(ui->transModeRadioFade, &QRadioButton::clicked, this, &MaterialWidget::transparencyModeChanged);
    connect(ui->transModeRadioNone, &QRadioButton::clicked, this, &MaterialWidget::transparencyModeChanged);
    connect(ui->useTextureAlphaCheck, &QCheckBox::clicked, this, &MaterialWidget::useAlphaFromTextureClicked);

    /// Normal section
    mNormalTexButton = new TextureButton(ui->normalTexButton);
    mNormalSpinSlider = new SpinSlider(ui->normalSlider, ui->normalSpin);
    connect(mNormalSpinSlider, &SpinSlider::valueChanged, this, &MaterialWidget::normalValueChanged);

    /// Roughness section
    mRoughnessTexButton = new TextureButton(ui->roughnessTexButton);
    mRoughnessSpinSlider = new SpinSlider(ui->roughnessSlider, ui->roughnessSpin);
    connect(mRoughnessSpinSlider, &SpinSlider::valueChanged, this, &MaterialWidget::roughnessValueChanged);

    /// Metallic section
    mMetallicTexButton = new TextureButton(ui->metallicTexButton);
    mMetallicSpinSlider = new SpinSlider(ui->metallicSlider, ui->metallicSpin);
    connect(mMetallicSpinSlider, &SpinSlider::valueChanged, this, &MaterialWidget::metallicValueChanged);
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
        show();
        updateMaterialCombo();
        updateOneDatablock();
    }
    else
    {
        hide();
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

void MaterialWidget::twoSidedClicked(bool b)
{
    Ogre::HlmsPbsDatablock* pbs = getCurrentDatablock();
    pbs->setTwoSidedLighting(b);
}

void MaterialWidget::diffuseColorButtonClicked()
{
    Ogre::HlmsPbsDatablock* pbs = getCurrentDatablock();
    Ogre::Vector3 originalColor = pbs->getDiffuse();
    QColor initColor = QColor::fromRgbF(originalColor.x, originalColor.y, originalColor.z);

    QColorDialog* dialog = new QColorDialog(initColor, this);
    dialog->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);

    connect(dialog, &QColorDialog::currentColorChanged, [&pbs] (const QColor& c)
    {
        pbs->setDiffuse(Ogre::Vector3(c.redF(), c.greenF(), c.blueF()));
    });

    connect(dialog, &QColorDialog::rejected, [&pbs, originalColor]
    {
        // back to the original color if users hit cancel button
        pbs->setDiffuse(originalColor);
    });

    connect(dialog, &QColorDialog::accepted, [this, pbs]
    {
        updateDiffuseGroup(pbs);
    });
    dialog->exec();
}

void MaterialWidget::diffuseBgColorButtonClicked()
{
    Ogre::HlmsPbsDatablock* pbs = getCurrentDatablock();
    Ogre::ColourValue originalColor = pbs->getBackgroundDiffuse();
    QColor initColor = QColor::fromRgbF(originalColor.r, originalColor.g, originalColor.b, originalColor.a);

    QColorDialog* dialog = new QColorDialog(initColor, this);
    dialog->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setOption(QColorDialog::ShowAlphaChannel, true);

    connect(dialog, &QColorDialog::currentColorChanged, [&pbs](const QColor& c)
    {
        pbs->setBackgroundDiffuse(Ogre::ColourValue(c.redF(), c.greenF(), c.blueF(), c.alphaF()));
    });

    connect(dialog, &QColorDialog::rejected, [&pbs, originalColor]
    {
        // back to the original color if users hit cancel button
        pbs->setBackgroundDiffuse(originalColor);
    });

    connect(dialog, &QColorDialog::accepted, [this, pbs]
    {
        updateDiffuseGroup(pbs);
    });

    dialog->exec();
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

void MaterialWidget::normalValueChanged(double value)
{
    Ogre::HlmsPbsDatablock* pbs = getCurrentDatablock();
    pbs->setNormalMapWeight(value);
}

void MaterialWidget::roughnessValueChanged(double value)
{
    QSignalBlocker b1(mRoughnessSpinSlider);
    Ogre::HlmsPbsDatablock* pbs = getCurrentDatablock();
    pbs->setRoughness(value);
}

void MaterialWidget::metallicValueChanged(double value)
{
    QSignalBlocker b1(mMetallicSpinSlider);
    Ogre::HlmsPbsDatablock* pbs = getCurrentDatablock();
    pbs->setMetalness(value);
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

    QSignalBlocker b1(ui->mtlNameCombo);
    ui->mtlNameCombo->clear();

    size_t num = mCurrentItem->getNumSubItems();
    for (size_t i = 0; i < num; ++i)
    {
        Ogre::HlmsDatablock* datablock = mCurrentItem->getSubItem(i)->getDatablock();

        QString name = QString::fromStdString(*datablock->getNameStr());
        QString displayText = QString("[%1] %2").arg(i).arg(name);
        ui->mtlNameCombo->addItem(displayText, QVariant(i));
    }
}

void MaterialWidget::updateOneDatablock()
{
    Ogre::HlmsPbsDatablock* pbs = getCurrentDatablock();
    Ogre::PolygonMode polygonMode = pbs->getMacroblock()->mPolygonMode;
    
    QSignalBlocker(ui->wireframeCheck);
    ui->wireframeCheck->setChecked(polygonMode == Ogre::PM_WIREFRAME);

    QSignalBlocker(ui->twoSidedCheck);
    ui->twoSidedCheck->setChecked(pbs->getTwoSidedLighting());

    updateDiffuseGroup(pbs);
    updateTransparencyGroup(pbs);
    updateNormalGroup(pbs);
    updateRoughnessGroup(pbs);
    updateMetallicGroup(pbs);
}

void MaterialWidget::updateDiffuseGroup(Ogre::HlmsPbsDatablock* pbs)
{
    Q_ASSERT(mCurrentItem);
    
    Ogre::TexturePtr tex = pbs->getTexture(Ogre::PBSM_DIFFUSE);
    mDiffuseTexButton->setTexture(pbs, Ogre::PBSM_DIFFUSE);
    
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

void MaterialWidget::updateNormalGroup(Ogre::HlmsPbsDatablock* pbs)
{
    Q_ASSERT(mCurrentItem);
    mNormalTexButton->setTexture(pbs, Ogre::PBSM_NORMAL);

    QSignalBlocker b1(mNormalSpinSlider);
    mNormalSpinSlider->setValue(pbs->getNormalMapWeight());
}

void MaterialWidget::updateRoughnessGroup(Ogre::HlmsPbsDatablock* pbs)
{
    Q_ASSERT(mCurrentItem);

    mRoughnessTexButton->setTexture(pbs, Ogre::PBSM_ROUGHNESS);

    QSignalBlocker b1(mRoughnessSpinSlider);
    float roughness = pbs->getRoughness();
    mRoughnessSpinSlider->setValue(roughness);
}

void MaterialWidget::updateMetallicGroup(Ogre::HlmsPbsDatablock* pbs)
{
    Q_ASSERT(mCurrentItem);

    mMetallicTexButton->setTexture(pbs, Ogre::PBSM_METALLIC);

    QSignalBlocker b1(mMetallicSpinSlider);
    float metallic = pbs->getMetalness();
    mMetallicSpinSlider->setValue(metallic);
}
