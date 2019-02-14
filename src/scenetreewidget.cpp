#include "scenetreewidget.h"
#include "ui_scenetreewidget.h"

#include "scenetreemodel.h"


SceneTreeWidget::SceneTreeWidget(QWidget* parent, OgreManager* ogre) : QWidget(parent),
    ui(new Ui::SceneTreeWidget)
{
    ui->setupUi(this);

    mOgre = ogre;
    mModel = new SceneTreeModel(this, ogre);
}

SceneTreeWidget::~SceneTreeWidget()
{
    delete ui;
}

void SceneTreeWidget::refresh()
{
    mModel->refresh();
    ui->treeView->setModel(mModel);
}

