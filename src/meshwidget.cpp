#include "stdafx.h"

#include "meshwidget.h"
#include "ui_meshwidget.h"

#include "OgreItem.h"
#include "OgreWireAabb.h"


MeshWidget::MeshWidget(QWidget* parent) : QWidget(parent)
{
    ui = new Ui::meshwidget;
    ui->setupUi(this);

    connect(ui->visibleCheck, &QCheckBox::clicked, this, &MeshWidget::visibleCheckClicked);
    connect(ui->boundingBoxCheck, &QCheckBox::clicked, this, &MeshWidget::boundingBoxCheckClicked);
}

MeshWidget::~MeshWidget()
{
    delete ui;
}

void MeshWidget::sceneNodeSelected(Ogre::SceneNode* node)
{
    Ogre::Item* item = getFirstItem(node);
    mCurrentItem = item;

    if (mCurrentItem)
    {
        show();
        updateMeshState(mCurrentItem);
    }
    else
    {
        hide();
    }
}

void MeshWidget::visibleCheckClicked(bool b)
{
    if (mCurrentItem)
    {
        mCurrentItem->setVisible(b);
    }
}

void MeshWidget::boundingBoxCheckClicked(bool b)
{
    if (!mCurrentItem) 
        return;

    Ogre::SceneManager* sceneMgr = Ogre::Root::getSingleton().getSceneManager("default");
    if (b)
    {
        Ogre::WireAabb* wireAabb = sceneMgr->createWireAabb();
        wireAabb->track(mCurrentItem);
        mAllWireAabb.push_back(wireAabb);
    }
    else
    {
        auto it = mAllWireAabb.begin();
        for (; it != mAllWireAabb.end(); ++it)
        {
            Ogre::WireAabb* wireAabb = *it;
            if (wireAabb->getTrackedObject() == mCurrentItem)
            {
                sceneMgr->destroyWireAabb(wireAabb);
                break;
            }
        }
        if (it != mAllWireAabb.end())
            mAllWireAabb.erase(it);
    }
}

void MeshWidget::updateMeshState(Ogre::Item*)
{
    if (mCurrentItem)
    {
        bool visible = mCurrentItem->isVisible();
        QSignalBlocker b1(ui->visibleCheck);
        ui->visibleCheck->setChecked(visible);

        bool isBoundingBoxShowing = false;
        for (Ogre::WireAabb* aabb : mAllWireAabb)
        {
            if (aabb->getTrackedObject() == mCurrentItem)
            {
                isBoundingBoxShowing = true;
                break;
            }
        }
        QSignalBlocker b2(ui->boundingBoxCheck);
        ui->boundingBoxCheck->setChecked(isBoundingBoxShowing);
    }
}

Ogre::Item* MeshWidget::getFirstItem(Ogre::SceneNode* node)
{
    if (node)
    {
        size_t num = node->numAttachedObjects();
        for (int i = 0; i < num; ++i)
        {
            Ogre::Item* item = dynamic_cast<Ogre::Item*>(node->getAttachedObject(i));
            if (item)
                return item;
        }
    }
    return nullptr;
}