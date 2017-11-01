#include "batchconversiondialog.h"
#include "ui_batchconversiondialog.h"

BatchConversionDialog::BatchConversionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BatchConversionDialog)
{
    ui->setupUi(this);
}

BatchConversionDialog::~BatchConversionDialog()
{
    delete ui;
}

void BatchConversionDialog::showEvent(QShowEvent* ev)
{
    // init things here
}
