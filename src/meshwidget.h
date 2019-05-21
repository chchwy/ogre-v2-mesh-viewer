#ifndef MESHWIDGET_H
#define MESHWIDGET_H

#include <QWidget>

namespace Ui {
class meshwidget;
}

namespace Ogre {
class Item;
class SceneNode;
}

class MeshWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MeshWidget(QWidget* parent = nullptr);
    ~MeshWidget();

    void sceneNodeSelected(Ogre::SceneNode* node);
    void visibleCheckClicked(bool b);
    void boundingBoxCheckClicked(bool b);

private:
    void updateMeshState(Ogre::Item*);
    Ogre::Item* getFirstItem(Ogre::SceneNode* node);

    Ogre::Item* mCurrentItem = nullptr;
    std::vector<Ogre::WireAabb*> mAllWireAabb;
    Ui::meshwidget* ui;
};

#endif // MESHWIDGET_H
