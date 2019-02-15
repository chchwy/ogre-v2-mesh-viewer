#include "scenetreewidget.h"
#include "ui_scenetreewidget.h"

#include "scenetreemodel.h"
#include "ogremanager.h"


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

void SceneTreeWidget::sceneNodeAdded(Ogre::SceneNode* node)
{
    Ogre::SceneNode* rootNode = mOgre->meshRootNode();

    if (rootNode == node->getParentSceneNode())
    {
        ui->treeView->model()->insertRow(0);
    }
    else
    {
        QModelIndex rootIndex = mModel->index(0, 0);
        QModelIndex index = lookForSceneNode(node, rootIndex);

        if (index.isValid())
            ui->treeView->model()->insertRow(0, index);
        //qDebug() << "Got it! " << index;
    }
}

QModelIndex SceneTreeWidget::lookForSceneNode(Ogre::SceneNode* tartgetNode, const QModelIndex& parent)
{
    auto node2 = static_cast<Ogre::SceneNode*>(parent.internalPointer());
    if (tartgetNode->getParentSceneNode() == node2)
    {
        return parent;
    }

    int i = 0;
    QModelIndex child = parent.child(i, 0);
    while(child.isValid())
    {
        QModelIndex idx = lookForSceneNode(tartgetNode, child);
        if (idx.isValid())
            return idx;

        i += 1;
        child = parent.child(i, 0);
    }
    return QModelIndex();
}

