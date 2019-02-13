#include "scenetreewidget.h"
#include "ui_scenetreewidget.h"

SceneTreeWidget::SceneTreeWidget(QWidget* parent) : QWidget(parent),
    ui(new Ui::SceneTreeWidget)
{
    ui->setupUi(this);
}

SceneTreeWidget::~SceneTreeWidget()
{
    delete ui;
}

void SceneTreeWidget::setOgre(OgreManager* ogre)
{
    Q_ASSERT(ogre != nullptr);
    mOgre = ogre;
}
