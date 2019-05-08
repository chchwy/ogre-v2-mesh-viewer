#include "stdafx.h"

#include "batchconversiondialog.h"
#include "ui_batchconversiondialog.h"

#include <QFileDialog>
#include <QSettings>
#include <QStandardPaths>
#include <QMessageBox>

#include "OgreMesh2Serializer.h"
#include "objimporter.h"


BatchConversionDialog::BatchConversionDialog(QWidget* parent) : QDialog(parent)
{
    ui = new Ui::BatchConversionDialog;
    ui->setupUi(this);

    connect(ui->AddFileButton, &QPushButton::clicked, this, &BatchConversionDialog::AddFileButtonClicked);
    connect(ui->ClearButton, &QPushButton::clicked, this, &BatchConversionDialog::ClearButtonClicked);
    connect(ui->BrowserOutputFolderButton, &QPushButton::clicked, this, &BatchConversionDialog::BrowserOutputFolderButtonClicked);
    connect(ui->ConvertButton, &QPushButton::clicked, this, &BatchConversionDialog::ConvertButtonClicked);
}

BatchConversionDialog::~BatchConversionDialog()
{
    delete ui;
}

void BatchConversionDialog::setSourceFolder(const QString& folder)
{
    mFolder = folder;
}

void BatchConversionDialog::showEvent(QShowEvent*)
{
    if (mInitialized == false)
    {
        QSettings settings("OgreV2ModelViewer", "OgreV2ModelViewer");
        QString myDocument = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0];
        QString outFolder = settings.value("BatchConverterOutputFolder", myDocument).toString();

        ui->OutputFolderText->setText(outFolder);

        mInitialized = true;
    }
}

void BatchConversionDialog::AddFileButtonClicked()
{
    QSettings settings("OgreV2ModelViewer", "OgreV2ModelViewer");

    QString myDocument = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0];
    QString defaultFolder = settings.value("BatchConverterAddFile", myDocument).toString();

    QStringList files = QFileDialog::getOpenFileNames(this,
                                                      "Select one or more files",
                                                      defaultFolder,
                                                      "Wavefront obj (*.obj)");
    ui->FileListWidget->addItems(files);

    if (!files.isEmpty())
    {
        QString sFile = files[0];
        QFileInfo info(sFile);
        settings.setValue("BatchConverterAddFile", info.absolutePath());
    }
}

void BatchConversionDialog::ClearButtonClicked()
{
    clearList();
}

void BatchConversionDialog::BrowserOutputFolderButtonClicked()
{
    QSettings settings("OgreV2ModelViewer", "OgreV2ModelViewer");

    QString sMyDoc = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0];
    QString sDefaultDir = settings.value("BatchConverterOutputFolder", sMyDoc).toString();

    QString sOutDir = QFileDialog::getExistingDirectory(this, "Select output folder", sDefaultDir);
    if (!sOutDir.isEmpty())
    {
        ui->OutputFolderText->setText(sOutDir);
    }

    if (!sOutDir.isEmpty())
    {
        QFileInfo info(sOutDir);
        settings.setValue("BatchConverterOutputFolder", info.absolutePath());
    }
}

void BatchConversionDialog::ConvertButtonClicked()
{
    bool bZupToYup = ui->ZupToYUpCheckBox->isChecked();
    QString sOutFolder = ui->OutputFolderText->text();

    int itemCount = ui->FileListWidget->count();

    for (int i = 0; i < itemCount; ++i)
    {
        QString inFile = ui->FileListWidget->item(i)->text();
        QString outFile = sOutFolder + "/" + QFileInfo(inFile).completeBaseName() + ".mesh";
        
        ObjImporter importer;
        importer.setZUpToYUp(bZupToYup);
        Ogre::MeshPtr meshPtr = importer.import(inFile, false);
        
        bool ok = writeMeshToDisk(meshPtr, outFile);

        qDebug() << "Src=" << inFile << ", Mesh=" << (!meshPtr.isNull()) << ", Dest=" << outFile;
    }

    QMessageBox::information(this, "Batch conversion", "Conversion done.");
    clearList();
}

bool BatchConversionDialog::writeMeshToDisk(const Ogre::MeshPtr mesh, const QString& outFile)
{
    Ogre::Root& root = Ogre::Root::getSingleton();
    Ogre::RenderSystem* renderSystem = root.getRenderSystem();
    Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();

    bool ok = false;

    try
    {
        Ogre::MeshSerializer meshSerializer2(vaoManager);
        meshSerializer2.exportMesh(mesh.getPointer(), outFile.toStdString());
        ok = true;
    }
    catch(...)
    {
        ok = false;
    }
    return (ok && QFile::exists(outFile));
}

void BatchConversionDialog::clearList()
{
    ui->FileListWidget->clear();
}
