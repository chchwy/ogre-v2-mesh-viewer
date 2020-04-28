#include "saveasdialog.h"
#include "ui_saveasdialog.h"

#include <QFileDialog>
#include <QSettings>

#include "OgreMesh2.h"
#include "OgreSubMesh2.h"
#include "OgreMesh2Serializer.h"
#include "OgreHlmsJson.h"
#include "OgreItem.h"

#include "ogremanager.h"


struct QVariantMeshWrapper
{
    QVariantMeshWrapper() {}
    Ogre::Item* item;
};

Q_DECLARE_METATYPE(QVariantMeshWrapper);


SaveAsDialog::SaveAsDialog(QWidget* parent, OgreManager* ogre) : QDialog(parent)
{
    ui = new Ui::SaveAsDialog;
    ui->setupUi(this);

    mOgre = ogre;
    ui->progressBar->setVisible(false);

    connect(ui->browseButton, &QPushButton::clicked, this, &SaveAsDialog::browseButtonClicked);
    connect(ui->saveButton, &QPushButton::clicked, this, &SaveAsDialog::saveButtonClicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &SaveAsDialog::cancelButtonClicked);
}

SaveAsDialog::~SaveAsDialog()
{
    delete ui;
}

void SaveAsDialog::showEvent(QShowEvent*)
{
    if (!mInitialized)
    {
        ui->savePathText->setText(mOutputFolder);

        listAllMeshesFromScene();
        mInitialized = true;
    }
}

void SaveAsDialog::browseButtonClicked()
{
    QString initialPath = ui->savePathText->text();
    QString folder = QFileDialog::getExistingDirectory(this, "Output path", initialPath);

    if (!folder.isEmpty())
    {
        ui->savePathText->setText(folder);
        mOutputFolder = folder;
    }
}

void SaveAsDialog::saveButtonClicked()
{
    lockListWidget();

    std::vector<Ogre::Item*> ogreItems;

    QListWidget* listWidget = ui->listWidget;
    for (int i = 0; i < listWidget->count(); ++i)
    {
        QListWidgetItem* item = listWidget->item(i);
        bool checked = (item->checkState() == Qt::Checked);
        if (checked)
        {
            auto wrapper = item->data(Qt::UserRole).value<QVariantMeshWrapper>();
            ogreItems.push_back(wrapper.item);
        }
    }

    ui->progressBar->setVisible(true);
    ui->progressBar->setMaximum(ogreItems.size());
    QApplication::processEvents(QEventLoop::DialogExec);

    saveOgreMeshes(ogreItems);

    Ogre::SceneNode* rootNode = mOgre->meshRootNode();
    std::vector<Ogre::SceneNode*> nodes;
    collectNodeRecursively(rootNode, nodes);

    saveLuaScript(nodes, ogreItems);

    QSettings settings("OgreV2ModelViewer", "OgreV2ModelViewer");
    settings.setValue("actionSaveMesh", mOutputFolder);

    accept();
}

void SaveAsDialog::cancelButtonClicked()
{
    reject();
}

void SaveAsDialog::listAllMeshesFromScene()
{
    Ogre::SceneNode* node = mOgre->meshRootNode();

    std::vector<Ogre::Item*> items;
    collectMeshRecursively(node, items);

    for (const Ogre::Item* m : items)
    {
        qDebug() << m->getName().c_str();
    }

    createListItems(items);
}

void SaveAsDialog::collectMeshRecursively(Ogre::SceneNode* node, std::vector<Ogre::Item*>& ogreItems)
{
    size_t count = node->numAttachedObjects();
    for (size_t i = 0; i < count; ++i)
    {
        Ogre::Item* item = dynamic_cast<Ogre::Item*>(node->getAttachedObject(i));
        if (item)
        {
            ogreItems.push_back(item);
        }
    }

    for (int i = 0; i < node->numChildren(); ++i)
    {
        Ogre::SceneNode* child = static_cast<Ogre::SceneNode*>(node->getChild(i));
        collectMeshRecursively(child, ogreItems);
    }
}

void SaveAsDialog::collectNodeRecursively(Ogre::SceneNode* node, std::vector<Ogre::SceneNode*>& ogreNodes)
{
    ogreNodes.push_back(node);

    for (int i = 0; i < node->numChildren(); ++i)
    {
        Ogre::SceneNode* child = static_cast<Ogre::SceneNode*>(node->getChild(i));
        collectNodeRecursively(child, ogreNodes);
    }
}

void SaveAsDialog::createListItems(const std::vector<Ogre::Item*>& ogreItems)
{
    for (Ogre::Item* i : ogreItems)
    {
        QString meshName = QString::fromStdString(i->getName());

        QListWidgetItem* listItem = new QListWidgetItem(meshName, ui->listWidget);
        listItem->setFlags(listItem->flags() | Qt::ItemIsUserCheckable);
        listItem->setCheckState(Qt::Checked);

        QVariantMeshWrapper wrapper;
        wrapper.item = i;
        listItem->setData(Qt::UserRole, QVariant::fromValue(wrapper));

        ui->listWidget->addItem(listItem);
    }

    //QSignalBlocker b(ui->selectAllCheckbox);
    //ui->selectAllCheckbox->setChecked(true);
}

void SaveAsDialog::lockListWidget()
{
    QListWidget* listWidget = ui->listWidget;
    for (int i = 0; i < listWidget->count(); ++i)
    {
        QListWidgetItem* item = listWidget->item(i);
        item->setFlags(Qt::NoItemFlags);
    }
    QApplication::processEvents(QEventLoop::DialogExec);
}

void SaveAsDialog::saveOgreMeshes(const std::vector<Ogre::Item*>& ogreItems)
{
    for (int i = 0; i < ogreItems.size(); ++i)
    {
        applySubMeshMaterialNames(ogreItems[i]);

        QString meshName = validateFileName(QString::fromStdString(ogreItems[i]->getMesh()->getName()));

        QString fullPath = QDir(mOutputFolder).filePath(meshName);
        Ogre::Mesh* mesh = ogreItems[i]->getMesh().get();

        Ogre::Root* root = mOgre->ogreRoot();
        Ogre::MeshSerializer meshSerializer2(root->getRenderSystem()->getVaoManager());
        meshSerializer2.exportMesh(mesh, fullPath.toStdString());

        saveHlmsJson(ogreItems[i]);

        ui->progressBar->setValue(i + 1);
        QApplication::processEvents(QEventLoop::DialogExec);
    }
}

void SaveAsDialog::saveHlmsJson(const Ogre::Item* ogreItem)
{
    auto hlmsManager = Ogre::Root::getSingleton().getHlmsManager();

    size_t numSubItem = ogreItem->getNumSubItems();
    for (int i = 0; i < numSubItem; ++i)
    {
        Ogre::HlmsDatablock* datablock = ogreItem->getSubItem(i)->getDatablock();

        Ogre::HlmsJson hlmsJson(hlmsManager, nullptr);
        std::string outJson;
        hlmsJson.saveMaterial(datablock, outJson, "");

        QString outputPath = QDir(mOutputFolder).filePath(datablock->getNameStr()->c_str());
        if (!outputPath.endsWith(".material.json"))
        {
            outputPath.append(".material.json");
        }

        QFile f(outputPath);
        if (f.open(QFile::WriteOnly))
        {
            QTextStream fout(&f);
            fout.setCodec("UTF-8");
            fout << QString::fromStdString(outJson);
        }
        f.close();

        qDebug() << "Write Hlms Json: " << outputPath;
    }
}

QString SaveAsDialog::validateFileName(QString fileName)
{
    if (fileName.endsWith(" (v1)"))
    {
        fileName = fileName.mid(0, fileName.size() - 5); // remove the (v1) part
    }

    if (fileName.endsWith(".mesh.xml"))
    {
        fileName = fileName.mid(0, fileName.size() - 4); // remove the .xml part
    }

    if (!fileName.endsWith(".mesh"))
    {
        fileName.append(".mesh"); // make sure the file extension is .mesh
    }
    return fileName;
}

void SaveAsDialog::applySubMeshMaterialNames(Ogre::Item* ogreItem)
{
    // Otherwise the exported mesh won't contain the material name.
    // Check OgreMesh2SerializerImpl.cpp line 269 (in MeshSerializerImpl::writeSubMesh())

    size_t numSubItems = ogreItem->getNumSubItems();
    for (size_t i = 0; i < numSubItems; ++i)
    {
        std::string datablockName = *ogreItem->getSubItem(i)->getDatablock()->getNameStr();
        ogreItem->getSubItem(i)->getSubMesh()->setMaterialName(datablockName);
    }
}

void SaveAsDialog::saveLuaScript(const std::vector<Ogre::SceneNode*>& nodes, const std::vector<Ogre::Item*>& ogreItems)
{
    // This function is just for my engine.
    QString fileName = "mesh_load2.lua";
    QString fullPath = QDir(mOutputFolder).filePath(fileName);

    QFile f(fullPath);
    if (!f.open(QFile::WriteOnly))
        return;

    QTextStream fout(&f);
    fout << "local node_list = {}\n";

    for (size_t i = 0; i < nodes.size(); ++i)
    {
        Ogre::SceneNode* node = nodes[i];
        Ogre::Vector3 pos = node->getPosition();

        const Ogre::Quaternion quaternion = node->getOrientation();
        Ogre::Matrix3 matrix3;
        quaternion.ToRotationMatrix(matrix3);

        Ogre::Radian rx, ry, rz;
        matrix3.ToEulerAnglesXYZ(rx, ry, rz);

        Ogre::Vector3 scale = node->getScale();
        
        QString parentNodeName = (node == mOgre->meshRootNode()) ? "main_building" : QString::fromStdString(node->getParent()->getName());

        QString line = QString("node_list[%1] = { name=\"%2\", pos={x=%3, y=%4, z=%5}, rot={x=%6, y=%7, z=%8}, scale={x=%9, y=%10, z=%11}, parent=\"%12\" }")
            .arg(i)
            .arg(node->getName().c_str())
            .arg(pos.x, 0, 'f', 4)
            .arg(pos.y, 0, 'f', 4)
            .arg(pos.z, 0, 'f', 4)
            .arg(rx.valueDegrees(), 0, 'f', 4)
            .arg(ry.valueDegrees(), 0, 'f', 4)
            .arg(rz.valueDegrees(), 0, 'f', 4)
            .arg(scale.x, 0, 'f', 4)
            .arg(scale.y, 0, 'f', 4)
            .arg(scale.z, 0, 'f', 4)
            .arg(parentNodeName);

        fout << line << "\n";
    }

    QString funcNodes = ""
        "\n"
        "function create_nodes() \n"
        "\n"
        "    for key, value in pairs(node_list) do \n"
        "        local node_name = value.name \n"
        "        log_info(key .. ': ' .. node_name) \n"
        "\n"
        "        local parent_node = get_object_by_name(value.parent) \n"
        "\n"
        "        local node = create_persistent_object('CSceneNodeOgre') \n"
        "        node:set_name(node_name) \n"
        "        node:set_ref('Parent', parent_node) \n"
        "        node:set_vec3('Position', { x = value.pos.x, y = value.pos.y, z = value.pos.z }) \n"
        "        node:set_vec3('Rotation', { x = value.rot.x, y = -value.rot.y, z = value.rot.z }) \n"
        "        node:set_vec3('Scale',    { x = value.scale.x, y = value.scale.y, z = value.scale.z }) \n"
        "        node:set_bool('Is Pickable', false) \n"
        "        node:signal_attribute('Parent') \n"
        "        node:signal_attribute('Name') \n"
        "        node:signal_attribute('Position') \n"
        "        node:signal_attribute('Rotation') \n"
        "        node:signal_attribute('Scale') \n"
        "    end \n"
        "end \n";

    fout << funcNodes;
    fout.flush();

    fout << "local mesh_list = {}\n";

    for (size_t i = 0; i < ogreItems.size(); ++i)
    {
        Ogre::Item* item = ogreItems[i];
        
        QString line = QString("mesh_list[%1] = { name=\"Mesh_%2\", mesh=\"%3\", parent=\"%4\" }")
            .arg(i)
            .arg(QString::fromStdString(item->getName()))
            .arg(QString::fromStdString(item->getMesh()->getName()))
            .arg(QString::fromStdString(item->getParentNode()->getName()));
        fout << line << "\n";
    }
    fout << "\n";

    QString func = "\n"
        "function create_meshes() \n"
        "    local mesh_map = {} \n"

        "    for key, value in pairs(mesh_list) do \n"
        "        local path = 'script/' .. value.mesh .. '.mesh' \n"
        "        log_info(path) \n"
        "\n"
        "        local parent_node = get_object_by_name(value.parent) \n"
        "\n"
        "        local mesh = mesh_map[value.mesh]\n"
        "        if mesh == nil then\n"
        "            mesh = create_persistent_object('COgreMesh')\n"
        "            mesh:set_string('Path', path)\n"
        "            mesh_map[value.mesh] = mesh\n"
        "        end\n"
        "\n"
        "        local node = create_persistent_object('CSceneNodeOgreMesh') \n"
        "        node:set_name(value.name) \n"
        "        node:set_ref('Ogre Mesh', mesh) \n"
        "        node:set_ref('Parent', parent_node) \n"
        "        node:set_bool('Is Pickable', false) \n"
        "        node:signal_attribute('Parent') \n"
        "        node:signal_attribute('Name') \n"
        "    end \n"
        "end \n";

    fout << func;

    fout << "\n"
        "function go()\n"
        "    create_nodes()\n"
        "    create_meshes()\n"
        "end";
        
    fout.flush();

    f.close();
}
