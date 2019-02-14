#ifndef SCENETREEWIDGET_H
#define SCENETREEWIDGET_H

#include <QWidget>

namespace Ui
{
class SceneTreeWidget;
}
class OgreManager;
class SceneTreeModel;

class SceneTreeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SceneTreeWidget(QWidget* parent, OgreManager* ogre);
    ~SceneTreeWidget();

    void refresh();

private:
    OgreManager* mOgre = nullptr;
    SceneTreeModel* mModel = nullptr;
    Ui::SceneTreeWidget* ui = nullptr;
};

#endif // SCENETREEWIDGET_H
