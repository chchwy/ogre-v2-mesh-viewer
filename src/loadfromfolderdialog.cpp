#include "stdafx.h"

#include "LoadFromFolderDialog.h"
#include "ui_loadfromfolderdialog.h"

#include <QStringList>
#include <QDirIterator>

#include "ogremanager.h"
#include "meshloader.h"


LoadFromFolderDialog::LoadFromFolderDialog(QWidget* parent, OgreManager* ogre) : QDialog(parent)
{
    ui = new Ui::LoadFromFolderDialog;
    ui->setupUi(this);

    mOgre = ogre;

    ui->progressBar->setVisible(false);

    connect(ui->selectAllCheckbox, &QCheckBox::clicked, this, &LoadFromFolderDialog::selectAllClicked);
    connect(ui->okButton, &QPushButton::clicked, this, &LoadFromFolderDialog::okButtonClicked);
}

LoadFromFolderDialog::~LoadFromFolderDialog()
{
    qDebug() << "delete dialog";
    delete ui;
}

void LoadFromFolderDialog::setSourceFolder(const QString& s)
{
    mFolder = s;
}

void LoadFromFolderDialog::listAllMeshesInFolder()
{
    QStringList meshList;

    QStringList nameFilters{ "*.mesh", "*.mesh.xml", "*.obj", "*.gltf", "*.glb" };
    QDirIterator it(mFolder, nameFilters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QString filePath = it.next();
        meshList.append(filePath);
    }
    qDebug() << meshList;
    createListItems(meshList);
}

void LoadFromFolderDialog::createListItems(const QStringList& meshList)
{
    for (QString s : meshList)
    {
        QString fullPath = s;

        s.replace(mFolder + "/", "");

        QListWidgetItem* item = new QListWidgetItem(s, ui->listWidget);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
        item->setData(Qt::UserRole, fullPath);

        ui->listWidget->addItem(item);
    }

    QSignalBlocker b(ui->selectAllCheckbox);
    ui->selectAllCheckbox->setChecked(true);
}

void LoadFromFolderDialog::lockListWidget()
{
    QListWidget* listWidget = ui->listWidget;
    for (int i = 0; i < listWidget->count(); ++i)
    {
        QListWidgetItem* item = listWidget->item(i);
        item->setFlags(Qt::NoItemFlags);
    }
    QApplication::processEvents(QEventLoop::DialogExec);
}

void LoadFromFolderDialog::selectAllClicked(bool b)
{
    QListWidget* listWidget = ui->listWidget;
    for (int i = 0; i < listWidget->count(); ++i)
    {
        QListWidgetItem* item = listWidget->item(i);

        Qt::CheckState state = (b) ? Qt::Checked : Qt::Unchecked;
        item->setCheckState(state);
    }
}

void LoadFromFolderDialog::okButtonClicked()
{
    lockListWidget();

    QStringList fileList;

    QListWidget* listWidget = ui->listWidget;
    for (int i = 0; i < listWidget->count(); ++i)
    {
        QListWidgetItem* item = listWidget->item(i);
        bool checked = (item->checkState() == Qt::Checked);
        if (checked)
        {
            fileList.append(item->data(Qt::UserRole).toString());
        }
    }

    ui->progressBar->setVisible(true);
    ui->progressBar->setMaximum(fileList.size());

    QApplication::processEvents(QEventLoop::DialogExec);

    for (int i = 0; i < fileList.size(); ++i)
    {
        mOgre->meshLoader()->load(fileList[i]);
        ui->progressBar->setValue(i + 1);

        QApplication::processEvents(QEventLoop::DialogExec);
    }

    accept();
}

void LoadFromFolderDialog::showEvent(QShowEvent* event)
{
    if (!mInitialized)
    {
        listAllMeshesInFolder();
        mInitialized = true;
    }
}
