#include "ambientlightwidget.h"
#include "ui_ambientlightwidget.h"

AmbientLightWidget::AmbientLightWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AmbientLightWidget)
{
    ui->setupUi(this);
}

AmbientLightWidget::~AmbientLightWidget()
{
    delete ui;
}
