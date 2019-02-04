#include "stdafx.h"

#include "LoadFromFolderDialog.h"
#include "ui_loadfromfolderdialog.h"

#include <QStringList>
#include <QDirIterator>


LoadFromFolderDialog::LoadFromFolderDialog(QWidget* parent) : QDialog(parent)
{
    ui = new Ui::LoadFromFolderDialog;
    ui->setupUi(this);
}


LoadFromFolderDialog::~LoadFromFolderDialog()
{
}

void LoadFromFolderDialog::setSourceFolder(const QString& s)
{
    mFolder = s;
}

void LoadFromFolderDialog::listAllMeshesInFolder()
{
    QStringList meshList;

    QDirIterator it(mFolder, QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        QString filePath = it.next();
        if (filePath.endsWith(".mesh"))
        {
            meshList.append(filePath);
        }
    }


}
