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
    void twoSidedClicked(bool b);

    void diffuseColorButtonClicked();
    void diffuseBgColorButtonClicked();

    void transparencyValueChanged(double value);
    void transparencyModeChanged();
    void useAlphaFromTextureClicked(bool b);
    void normalValueChanged(double value);
    void roughnessValueChanged(double value);
    void metallicValueChanged(double value);

private:
    Ogre::Item* getFirstItem(Ogre::SceneNode* node);
    Ogre::HlmsPbsDatablock* getCurrentDatablock();

    void updateMaterialCombo();
    void updateOneDatablock();
    void updateDiffuseGroup(Ogre::HlmsPbsDatablock*);
    void updateTransparencyGroup(Ogre::HlmsPbsDatablock*);
    void updateNormalGroup(Ogre::HlmsPbsDatablock*);
    void updateRoughnessGroup(Ogre::HlmsPbsDatablock*);
    void updateMetallicGroup(Ogre::HlmsPbsDatablock*);

private:
    Ogre::Item* mCurrentItem = nullptr;
    SpinSlider* mTransparencySpinSlider = nullptr;
    SpinSlider* mNormalSpinSlider = nullptr;
    SpinSlider* mRoughnessSpinSlider = nullptr;
    SpinSlider* mMetallicSpinSlider = nullptr;
    TextureButton* mDiffuseTexButton = nullptr;
    TextureButton* mNormalTexButton = nullptr;
    TextureButton* mRoughnessTexButton = nullptr;
    TextureButton* mMetallicTexButton = nullptr;

    Ui::MaterialWidget* ui = nullptr;
};

#endif // MATERIALWIDGET_H
