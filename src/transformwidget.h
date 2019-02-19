#ifndef TRANSFORMWIDGET_H
#define TRANSFORMWIDGET_H

#include <QWidget>

namespace Ui {
class TransformWidget;
}

class TransformWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TransformWidget(QWidget* parent = nullptr);
    ~TransformWidget();

public slots:
    void sceneNodeSelected(Ogre::SceneNode*);

private:
    Ogre::SceneNode* mCurrentNode = nullptr;
    Ui::TransformWidget* ui;
};

#endif // TRANSFORMWIDGET_H
