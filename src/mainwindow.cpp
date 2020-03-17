/****************************************************************************
**
** Copyright (C) 2016 This file is generated by the Magus toolkit
** Copyright (C) 2016-2019 Matt Chiawen Chang
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
****************************************************************************/


// Include
#include "stdafx.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTimer>
#include <QFile>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include <QDockWidget>
#include <QSettings>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDirIterator>
#include <QProgressDialog>

#include "OgreMesh2Serializer.h"

#include "ogremanager.h"
#include "ogrewidget.h"
#include "meshloader.h"
#include "objexporter.h"
#include "batchconversiondialog.h"
#include "loadfromfolderdialog.h"
#include "lightwidget.h"
#include "scenetreewidget.h"
#include "transformwidget.h"
#include "materialwidget.h"
#include "meshwidget.h"
#include "inspector.h"
#include "cameracontroller.h"
#include "saveasdialog.h"


MainWindow::MainWindow()
{
    ui = new Ui::MainWindow;
    ui->setupUi(this);

    mOgreManager = new OgreManager;
    mOgreWidget = new OgreWidget;
    setCentralWidget(mOgreWidget);

    mOgreManager->registerOgreWidget(mOgreWidget);
    mOgreWidget->createRenderWindow(mOgreManager);

    mOgreManager->initialize();
    mOgreWidget->createCompositor();

    mOgreManager->createSubcomponents();

    createDockWindows();

    // Set the title
    setWindowTitle("Ogre v2 Mesh Viewer [" APP_VERSION "]");
    readSettings();

    connect(mOgreManager, &OgreManager::sceneCreated, this, &MainWindow::onSceneLoaded);
    connect(mOgreManager->meshLoader(), &MeshLoader::sceneNodeAdded, mSeceneWidget, &SceneTreeWidget::sceneNodeAdded);
    connect(mSeceneWidget, &SceneTreeWidget::sceneNodeSelected, mTransformWidget, &TransformWidget::sceneNodeSelected);
    connect(mSeceneWidget, &SceneTreeWidget::sceneNodeSelected, mMaterialWidget, &MaterialWidget::sceneNodeSelected);
    connect(mSeceneWidget, &SceneTreeWidget::sceneNodeSelected, mMeshWidget, &MeshWidget::sceneNodeSelected);

    // actions
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::actionOpen);
    connect(ui->actionSaveOgreMesh, &QAction::triggered, this, &MainWindow::actionSaveMesh);
    connect(ui->actionBatchConverter, &QAction::triggered, this, &MainWindow::actionBatchConverter);
    connect(ui->actionLoad_From_Folder, &QAction::triggered, this, &MainWindow::actionLoadFromFolder);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::doQuitMenuAction);
    connect(ui->actionResetCamera, &QAction::triggered, this, &MainWindow::actionResetCamera);
    connect(ui->actionBgEnv, &QAction::triggered, this, &MainWindow::actionBgEnvironment);
    connect(ui->actionBgIrradiance, &QAction::triggered, this, &MainWindow::actionBgIrradiance);
    connect(ui->actionBgBlack, &QAction::triggered, this, &MainWindow::actionBgBlack);

    // setup the timer
    mTimer = new QTimer(this);
    mTimer->setInterval(16);
    connect(mTimer, &QTimer::timeout, this, &MainWindow::Tick);

    mUserDocumentPath = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0];
}

MainWindow::~MainWindow()
{
    delete mOgreManager;
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    mIsClosing = true;

    mTimer->stop();

    QSettings settings("OgreV2ModelViewer", "OgreV2ModelViewer");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    QMainWindow::closeEvent(event);
}

void MainWindow::readSettings()
{
    QSettings settings("OgreV2ModelViewer", "OgreV2ModelViewer");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}

void MainWindow::onSceneLoaded()
{
    QDir meshDir(QApplication::applicationDirPath());
    meshDir.cd("../mesh");

    qDebug() << "Search for meshes in" << meshDir.absolutePath();
    QDirIterator d(meshDir.absolutePath(), { "*.mesh", "*.glb", "*.gltf" });
    while (d.hasNext())
    {
        QString meshFile = d.next();
        mOgreManager->meshLoader()->load(meshFile);
    }

    mOgreManager->setIrradianceBackground();
    mSeceneWidget->sceneLoaded();
}

void MainWindow::Tick()
{
    if (mOgreManager)
        mOgreManager->render();
}

void MainWindow::createDockWindows()
{
    /*
    mLightWidget = new LightWidget(this);
    mLightWidget->init(mOgreManager);
    QDockWidget* dockWidget = new QDockWidget("Lights", this);
    dockWidget->setWidget(mLightWidget);
    addDockWidget(Qt::LeftDockWidgetArea, dockWidget);
    */

    mSeceneWidget = new SceneTreeWidget(this, mOgreManager);
    QDockWidget* sceneTreeDock = new QDockWidget("Scene Tree", this);
    sceneTreeDock->setWidget(mSeceneWidget);
    addDockWidget(Qt::LeftDockWidgetArea, sceneTreeDock);

    mInspector = new Inspector(this);
    QDockWidget* inspectorDock = new QDockWidget("Inspector", this);
    inspectorDock->setWidget(mInspector);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
    {
        mTransformWidget = new TransformWidget;
        mInspector->addWidget(mTransformWidget);

        mMeshWidget = new MeshWidget;
        mInspector->addWidget(mMeshWidget);

        mMaterialWidget = new MaterialWidget;
        mInspector->addWidget(mMaterialWidget);
    }
    mInspector->endAddWidget();
}

void MainWindow::startTimer()
{
    mOgreManager->createScene();
    mTimer->start();
}

void MainWindow::doQuitMenuAction()
{
    close();
}

void MainWindow::actionOpen()
{
    mTimer->stop();
    ON_SCOPE_EXIT(mTimer->start());

    QSettings settings("OgreV2ModelViewer", "OgreV2ModelViewer");
    QString lastOpenLocation = settings.value("actionOpen", mUserDocumentPath).toString();

    QString fileFilters =
        "All supported formats (*.mesh *.mesh.xml *.obj *.gltf *.glb);;"
        "Ogre Mesh (*.mesh *.mesh.xml);;"
        "Wavefront Obj (*.obj);;"
        "glTF (*.gltf *.glb);;All Files (*)";

    QString sMeshFileName = QFileDialog::getOpenFileName(this, "Open", lastOpenLocation, fileFilters);
    if (sMeshFileName.isEmpty())
    {
        return;
    }

    Q_ASSERT(QFile::exists(sMeshFileName));

    bool bConversionZupToYup = false;
    if (sMeshFileName.endsWith(".obj"))
    {
        QMessageBox::StandardButton ret = QMessageBox::question(this, "Convert Z-up into Y-up",
                                                                "Do you want to do Z-up to Y-up conversion?");
        bConversionZupToYup = (ret == QMessageBox::Yes);
    }

    if (mFirstLoad)
    {
        mSeceneWidget->clear();
        mOgreManager->clearScene(); // clear the sample model
    }

    QProgressDialog progress(QString("Loading %1").arg(sMeshFileName), "Cancel", 0, 0, this); // Cancel is not working atm, but whatever
    progress.setWindowFlags(progress.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    QApplication::processEvents();
    {
        QFileInfo info(sMeshFileName);
        settings.setValue("actionOpen", info.absolutePath());

        auto& manager = Ogre::ResourceGroupManager::getSingleton();
        manager.addResourceLocation(info.absolutePath().toStdString(), "FileSystem", "ViewerResc");

        /*
        auto allMaterials = manager.findResourceNames("ViewerResc", "*.material.json");
        for (const std::string& sMtlName : *allMaterials)
        {
            Ogre::Root::getSingleton().getHlmsManager()->loadMaterials(sMtlName, "ViewerResc", nullptr, "");
        }*/

        MeshLoader* meshLoader = mOgreManager->meshLoader();
        meshLoader->enableZupToYupConversion(bConversionZupToYup);

        bool ok = meshLoader->load(sMeshFileName);
        if (!ok)
        {
            qDebug() << "Mesh load failed";
            QMessageBox::information(this, "Error", "Filed to open" + sMeshFileName);
        }
    }
    progress.cancel();

    if (mFirstLoad)
    {
        mSeceneWidget->sceneLoaded();
        mFirstLoad = false;
    }
}

void MainWindow::actionSaveMesh()
{
    mTimer->stop();
    ON_SCOPE_EXIT(mTimer->start());

    QSettings settings("OgreV2ModelViewer", "OgreV2ModelViewer");
    QString sLastOpenLocation = settings.value("actionSaveMesh", mUserDocumentPath).toString();

    SaveAsDialog* dialog = new SaveAsDialog(this, mOgreManager);
    dialog->setFolder(sLastOpenLocation);
    dialog->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->open();
}

void MainWindow::actionExportObj()
{
    mTimer->stop();
    ON_SCOPE_EXIT(mTimer->start());

    QSettings settings("OgreV2ModelViewer", "OgreV2ModelViewer");
    QString sLastOpenLocation = settings.value("actionExportObj", mUserDocumentPath).toString();

    QString sObjFileName = QFileDialog::getSaveFileName(this, "Export Obj",
                                                        sLastOpenLocation + "/a.obj",
                                                        "Wavefront obj (*.obj)");
    if (sObjFileName.isEmpty())
    {
        return;
    }
    if (QFile::exists(sObjFileName)) QFile::remove(sObjFileName);

    Q_ASSERT(!QFile::exists(sObjFileName));

    QFileInfo info(sObjFileName);
    settings.setValue("actionExportObj", info.absolutePath());

    Ogre::Mesh* mesh = mOgreManager->currentMesh();
    if (mesh != nullptr)
    {
        ObjExporter objExporter;
        bool ok = objExporter.writeToFile(mesh, sObjFileName);

        qDebug() << "Obj=" << sObjFileName << ", Success=" << ok;

        if (!ok)
        {
            qDebug() << "Failed to export obj model.";
            QMessageBox::information(this, "Error", "Filed to export obj model");
        }
    }
}

void MainWindow::actionLoadFromFolder()
{
    mTimer->stop();
    ON_SCOPE_EXIT(mTimer->start());

    QSettings settings("OgreV2ModelViewer", "OgreV2ModelViewer");
    QString initialPath = settings.value("actionLoadFromFolder", mUserDocumentPath).toString();
    QString folder = QFileDialog::getExistingDirectory(this, "Open a folder", initialPath);

    if (folder.isEmpty())
    {
        return;
    }

    settings.setValue("actionLoadFromFolder", folder);
    settings.sync();

    if (mFirstLoad)
    {
        mSeceneWidget->clear();
        mOgreManager->clearScene(); // clear the sample model
    }

    LoadFromFolderDialog* dialog = new LoadFromFolderDialog(this, mOgreManager);
    dialog->setSourceFolder(folder);
    dialog->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->open();

    connect(dialog, &LoadFromFolderDialog::finished, [this](int result)
    {
        mSeceneWidget->sceneLoaded();
        mFirstLoad = false;
    });
}

void MainWindow::actionBatchConverter()
{
    mTimer->stop();
    ON_SCOPE_EXIT(mTimer->start());

    BatchConversionDialog* dialog = new BatchConversionDialog(this);
    dialog->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->open();
}

void MainWindow::actionResetCamera()
{
    mOgreWidget->cameraController()->reset();
}

void MainWindow::actionBgIrradiance()
{
    mOgreManager->setIrradianceBackground();
}

void MainWindow::actionBgEnvironment()
{
    mOgreManager->setEnvironmentBackground();
}

void MainWindow::actionBgBlack()
{
    mOgreManager->setBlackBackground();
}
