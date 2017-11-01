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
    explicit BatchConversionDialog(QWidget* parent = 0);
    ~BatchConversionDialog();

protected:
    void showEvent(QShowEvent* ev) override;

private:
    void AddFileButtonClicked();
    void ClearButtonClicked();
    void BrowserOutputFolderButtonClicked();
    void ConvertButtonClicked();

private:
    Ui::BatchConversionDialog *ui;
};

#endif // BATCHCONVERSIONDIALOG_H
