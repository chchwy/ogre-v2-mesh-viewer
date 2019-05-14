#ifndef SAVEASDIALOG_H
#define SAVEASDIALOG_H

#include <QDialog>


class OgreManager;

namespace Ogre {
class Item;
class SceneNode;
}

namespace Ui {
class SaveAsDialog;
}


class SaveAsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SaveAsDialog(QWidget* parent, OgreManager* ogre);
    ~SaveAsDialog();

    void setFolder(QString s) { mOutputFolder = s; }

protected:
    void showEvent(QShowEvent* event) override;

private:
    void browseButtonClicked();
    void saveButtonClicked();
    void cancelButtonClicked();

    void listAllMeshesFromScene();
    void collectMeshRecursively(Ogre::SceneNode* node, std::vector<Ogre::Item*>& ogreItems);
    void createListItems(const std::vector<Ogre::Item*>& ogreItems);
    void lockListWidget();

    void saveOgreMeshes(const std::vector<Ogre::Item*>& ogreItems);
    void saveHlmsJson(const Ogre::Item* ogreItem);
    QString validateFileName(QString fileName);
    void applySubMeshMaterialNames(Ogre::Item* ogreItem);

    OgreManager* mOgre = nullptr;
    bool mInitialized = false;
    QString mOutputFolder;

    Ui::SaveAsDialog *ui;
};

#endif // SAVEASDIALOG_H
