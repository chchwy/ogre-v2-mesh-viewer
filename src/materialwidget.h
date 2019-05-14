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
}

class MaterialWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MaterialWidget(QWidget* parent = nullptr);
    ~MaterialWidget();

    void sceneNodeSelected(Ogre::SceneNode* node);

private:
    Ogre::Item* getFirstItem(Ogre::SceneNode* node);
    void updateMaterialCombo();

private:
    Ogre::Item* mCurrentItem = nullptr;
    Ui::MaterialWidget* ui = nullptr;
};

#endif // MATERIALWIDGET_H
