#ifndef AMBIENTLIGHTWIDGET_H
#define AMBIENTLIGHTWIDGET_H

#include <QWidget>

namespace Ui {
class AmbientLightWidget;
}

class AmbientLightWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AmbientLightWidget(QWidget *parent = 0);
    ~AmbientLightWidget();

private:
    Ui::AmbientLightWidget *ui;
};

#endif // AMBIENTLIGHTWIDGET_H
