#pragma once

#include <QDialog>

namespace Ui
{
class LoadFromFolderDialog;
}
class OgreManager;


class LoadFromFolderDialog : public QDialog
{
    Q_OBJECT
public:
    LoadFromFolderDialog(QWidget* parent, OgreManager* ogre);
    ~LoadFromFolderDialog();

    void setSourceFolder(const QString& s);
    void listAllMeshesInFolder();
    void createListItems(const QStringList& meshList);

protected:
    void showEvent(QShowEvent* event) override;

private:
    void selectAllClicked(bool b);
    void okButtonClicked();

    QString mFolder;
    bool mInitialized = false;

    OgreManager* mOgre = nullptr;

    Ui::LoadFromFolderDialog* ui = nullptr;
};

