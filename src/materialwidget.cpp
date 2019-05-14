#include "materialwidget.h"
#include "ui_materialwidget.h"

MaterialWidget::MaterialWidget(QWidget* parent) : QWidget(parent)
{
    ui = new Ui::MaterialWidget;
    ui->setupUi(this);
}

MaterialWidget::~MaterialWidget()
{
    delete ui;
}

void MaterialWidget::sceneNodeSelected(Ogre::SceneNode* node)
{
    Ogre::Item* item = getFirstItem(node);
    mCurrentItem = item;

    if (mCurrentItem)
    {
        updateMaterialCombo();
    }
}

Ogre::Item* MaterialWidget::getFirstItem(Ogre::SceneNode* node)
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

void MaterialWidget::updateMaterialCombo()
{
    Q_ASSERT(mCurrentItem);

    ui->mtlNameCombo->clear();

    int num = mCurrentItem->getNumSubItems();
    for (int i = 0; i < num; ++i)
    {
        Ogre::HlmsDatablock* datablock = mCurrentItem->getSubItem(i)->getDatablock();

        QString name = QString::fromStdString(*datablock->getNameStr());
        ui->mtlNameCombo->addItem(name, QVariant(i));
    }
}
