#ifndef MATERIALWIDGET_H
#define MATERIALWIDGET_H

#include <QWidget>

namespace Ui
{
class MaterialWidget;
}

namespace Ogre
{
class SceneNode;
class Item;
class HlmsPbsDatablock;
}

class SpinSlider;
class TextureButton;

class MaterialWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MaterialWidget(QWidget* parent = nullptr);
    ~MaterialWidget();

public slots:
    void sceneNodeSelected(Ogre::SceneNode* node);
    void materialComboIndexChanged(int i);
    void wireFrameClicked(bool b);

    void transparencyValueChanged(double value);
    void transparencyModeChanged();
    void useAlphaFromTextureClicked(bool b);
    void roughnessValueChanged(double value);
    void metallicValueChanged(double value);

private:
    Ogre::Item* getFirstItem(Ogre::SceneNode* node);
    Ogre::HlmsPbsDatablock* getCurrentDatablock();

    void updateMaterialCombo();
    void updateOneDatablock();
    void updateDiffuseGroup(Ogre::HlmsPbsDatablock*);
    void updateTransparencyGroup(Ogre::HlmsPbsDatablock*);
    void updateRoughnessGroup(Ogre::HlmsPbsDatablock*);
    void updateMetallicGroup(Ogre::HlmsPbsDatablock*);

    void enableAll();
    void disableAll();

    QImage::Format toQtImageFormat(Ogre::PixelFormat);

private:
    Ogre::Item* mCurrentItem = nullptr;
    SpinSlider* mTransparencySpinSlider = nullptr;
    SpinSlider* mRoughnessSpinSlider = nullptr;
    SpinSlider* mMetallicSpinSlider = nullptr;
    TextureButton* mDiffuseTexButton = nullptr;

    Ui::MaterialWidget* ui = nullptr;
};

#endif // MATERIALWIDGET_H
