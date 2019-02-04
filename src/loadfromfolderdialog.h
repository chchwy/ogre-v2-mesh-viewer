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


private:
    Ui::LoadFromFolderDialog* ui = nullptr;
};

