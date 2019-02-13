#ifndef SCENETREEWIDGET_H
#define SCENETREEWIDGET_H

#include <QWidget>

namespace Ui
{
class SceneTreeWidget;
}

class SceneTreeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SceneTreeWidget(QWidget* parent = nullptr);
    ~SceneTreeWidget();

private:
    Ui::SceneTreeWidget* ui = nullptr;
};

#endif // SCENETREEWIDGET_H
