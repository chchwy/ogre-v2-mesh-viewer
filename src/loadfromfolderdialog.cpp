#include "stdafx.h"

#include "LoadFromFolderDialog.h"
#include "ui_loadfromfolderdialog.h"



LoadFromFolderDialog::LoadFromFolderDialog(QWidget* parent) : QDialog(parent)
{
    ui = new Ui::LoadFromFolderDialog;
    ui->setupUi(this);
}


LoadFromFolderDialog::~LoadFromFolderDialog()
{
}
