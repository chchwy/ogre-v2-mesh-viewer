#ifndef SCENETREEWIDGET_H
#define SCENETREEWIDGET_H

#include <QWidget>

namespace Ui
{
class SceneTreeWidget;
}
class OgreManager;


class SceneTreeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SceneTreeWidget(QWidget* parent = nullptr);
    ~SceneTreeWidget();

    void setOgre(OgreManager* ogre);

private:
    OgreManager* mOgre = nullptr;

    Ui::SceneTreeWidget* ui = nullptr;
};

#endif // SCENETREEWIDGET_H
