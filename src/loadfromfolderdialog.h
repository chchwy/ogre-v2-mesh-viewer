#pragma once

#include <QDialog>

namespace Ui
{
class LoadFromFolderDialog;
}

class LoadFromFolderDialog : public QDialog
{
    Q_OBJECT
public:
    LoadFromFolderDialog(QWidget* parent);
    ~LoadFromFolderDialog();

    void setSourceFolder(const QString& s);
    void listAllMeshesInFolder();

private:
    QString mFolder;

    Ui::LoadFromFolderDialog* ui = nullptr;
};

