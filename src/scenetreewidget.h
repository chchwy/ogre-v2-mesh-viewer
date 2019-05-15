#ifndef SCENETREEWIDGET_H
#define SCENETREEWIDGET_H

#include <QWidget>

namespace Ui
{
class SceneTreeWidget;
}
namespace Ogre
{
class SceneNode;
}

class OgreManager;
class SceneTreeModel;

class SceneTreeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SceneTreeWidget(QWidget* parent, OgreManager* ogre);
    ~SceneTreeWidget();

    void clear();
    void sceneLoaded();

    void treeViewClicked(QModelIndex index);
    void sceneNodeAdded(Ogre::SceneNode*);

signals:
    void sceneNodeSelected(Ogre::SceneNode*);

private:
    QModelIndex lookForSceneNode(Ogre::SceneNode* node, const QModelIndex& parent);

    OgreManager* mOgre = nullptr;
    SceneTreeModel* mModel = nullptr;
    Ui::SceneTreeWidget* ui = nullptr;
};

#endif // SCENETREEWIDGET_H
