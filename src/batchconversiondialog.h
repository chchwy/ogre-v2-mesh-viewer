#ifndef BATCHCONVERSIONDIALOG_H
#define BATCHCONVERSIONDIALOG_H

#include <QDialog>

namespace Ui {
class BatchConversionDialog;
}

class BatchConversionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BatchConversionDialog(QWidget* parent = nullptr);
    ~BatchConversionDialog();

    void setSourceFolder(const QString& folder);

protected:
    void showEvent(QShowEvent* ev) override;

private:
    void AddFileButtonClicked();
    void ClearButtonClicked();
    void BrowserOutputFolderButtonClicked();
    void ConvertButtonClicked();

    bool writeMeshToDisk(const Ogre::MeshPtr mesh, const QString& outFile);
    void clearList();

private:
    bool mInitialized = false;
    QString mFolder;

    Ui::BatchConversionDialog *ui;
};

#endif // BATCHCONVERSIONDIALOG_H
