#include "saveasdialog.h"
#include "ui_saveasdialog.h"

#include <QFileDialog>
#include "OgreMesh2.h"
#include "OgreMesh2Serializer.h"

#include "ogremanager.h"


struct QVariantMeshWrapper
{
    QVariantMeshWrapper() {}
    Ogre::Item* item;
};

Q_DECLARE_METATYPE(QVariantMeshWrapper);


SaveAsDialog::SaveAsDialog(QWidget* parent, OgreManager* ogre) : QDialog(parent)
{
    ui = new Ui::SaveAsDialog;
    ui->setupUi(this);

    mOgre = ogre;
    ui->progressBar->setVisible(false);

    connect(ui->browseButton, &QPushButton::clicked, this, &SaveAsDialog::browseButtonClicked);
    connect(ui->saveButton, &QPushButton::clicked, this, &SaveAsDialog::saveButtonClicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &SaveAsDialog::cancelButtonClicked);
}

SaveAsDialog::~SaveAsDialog()
{
    delete ui;
}

void SaveAsDialog::showEvent(QShowEvent*)
{
    if (!mInitialized)
    {
        ui->savePathText->setText(mOutputFolder);

        listAllMeshesFromScene();
        mInitialized = true;
    }
}

void SaveAsDialog::browseButtonClicked()
{
    QString initialPath = ui->savePathText->text();
    QString folder = QFileDialog::getExistingDirectory(this, "Output path", initialPath);

    if (!folder.isEmpty())
    {
        ui->savePathText->setText(folder);
        mOutputFolder = folder;
    }
}

void SaveAsDialog::saveButtonClicked()
{
    std::vector<Ogre::Item*> ogreItems;

    QListWidget* listWidget = ui->listWidget;
    for (int i = 0; i < listWidget->count(); ++i)
    {
        QListWidgetItem* item = listWidget->item(i);
        bool checked = (item->checkState() == Qt::Checked);
        if (checked)
        {
            auto wrapper = item->data(Qt::UserRole).value<QVariantMeshWrapper>();
            ogreItems.push_back(wrapper.item);
        }
    }

    ui->progressBar->setVisible(true);
    ui->progressBar->setMaximum(ogreItems.size());

    QApplication::processEvents(QEventLoop::DialogExec);

    for (int i = 0; i < ogreItems.size(); ++i)
    {
        QString meshName = QString::fromStdString(ogreItems[i]->getMesh()->getName());
        if (meshName.endsWith(" (v1)"))
        {
            meshName = meshName.mid(0, meshName.size() - 5); // remove the (v1) part
        }

        if (meshName.endsWith(".mesh.xml"))
        {
            meshName = meshName.mid(0, meshName.size() - 4); // remove the .xml part
        }

        if (!meshName.endsWith(".mesh"))
        {
            meshName.append(".mesh"); // make sure the file extension is .mesh
        }

        QString fullPath = QDir(mOutputFolder).filePath(meshName);
        Ogre::Mesh* mesh = ogreItems[i]->getMesh().get();

        Ogre::Root* root = mOgre->ogreRoot();
        Ogre::MeshSerializer meshSerializer2(root->getRenderSystem()->getVaoManager());
        meshSerializer2.exportMesh(mesh, fullPath.toStdString());

        ui->progressBar->setValue(i + 1);
        QApplication::processEvents(QEventLoop::DialogExec);
    }

    accept();
}

void SaveAsDialog::cancelButtonClicked()
{
    reject();
}

void SaveAsDialog::listAllMeshesFromScene()
{
    Ogre::SceneNode* node = mOgre->meshRootNode();

    std::vector<Ogre::Item*> items;
    collectMeshRecursively(node, items);

    for (const Ogre::Item* m : items)
    {
        qDebug() << m->getName().c_str();
    }

    createListItems(items);
}

void SaveAsDialog::collectMeshRecursively(Ogre::SceneNode* node, std::vector<Ogre::Item*>& ogreItems)
{
    size_t count = node->numAttachedObjects();
    for (size_t i = 0; i < count; ++i)
    {
        Ogre::Item* item = dynamic_cast<Ogre::Item*>(node->getAttachedObject(i));
        if (item)
        {
            ogreItems.push_back(item);
        }
    }

    for (int i = 0; i < node->numChildren(); ++i)
    {
        Ogre::SceneNode* child = static_cast<Ogre::SceneNode*>(node->getChild(i));
        collectMeshRecursively(child, ogreItems);
    }
}

void SaveAsDialog::createListItems(const std::vector<Ogre::Item*>& ogreItems)
{
    for (Ogre::Item* i : ogreItems)
    {
        QString meshName = QString::fromStdString(i->getMesh()->getName());

        QListWidgetItem* listItem = new QListWidgetItem(meshName, ui->listWidget);
        listItem->setFlags(listItem->flags() | Qt::ItemIsUserCheckable);
        listItem->setCheckState(Qt::Checked);

        QVariantMeshWrapper wrapper;
        wrapper.item = i;
        listItem->setData(Qt::UserRole, QVariant::fromValue(wrapper));

        ui->listWidget->addItem(listItem);
    }

    //QSignalBlocker b(ui->selectAllCheckbox);
    //ui->selectAllCheckbox->setChecked(true);
}
