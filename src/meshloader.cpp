#include "stdafx.h"

#include "meshloader.h"

// ogre headers
#include <OgreItem.h>
#include <OgreHlmsPbsDatablock.h>
#include <OgreMeshManager2.h>
#include <OgreMeshManager.h>
#include <OgreMesh2.h>
#include <OgreWireAabb.h>

// qt headers
#include <QFile>
#include <QFileInfo>

#include "ogremanager.h"
#include "objimporter.h"
#include "OgreXML/OgreXMLMeshSerializer.h"
#include "OgreGLTF/Ogre_glTF.hpp"


MeshLoader::MeshLoader(QObject* parent, OgreManager* ogre) : QObject(parent)
{
    mOgre = ogre;
}

MeshLoader::~MeshLoader()
{
}

bool MeshLoader::load(QString filePath)
{
    if (!QFile::exists(filePath))
    {
        qDebug() << "File doesn't exist:" << filePath;
        return false;
    }

    if (filePath.endsWith(".mesh.xml"))
    {
        return loadOgreMeshXML(filePath);
    }
    else if (filePath.endsWith(".mesh"))
    {
        return loadOgreMesh(filePath);
    }
    else if (filePath.endsWith(".gltf") || filePath.endsWith(".glb"))
    {
        return loadGLTF(filePath);
    }
    else if (filePath.endsWith(".obj"))
    {
        return loadWavefrontObj(filePath);
    }
    qDebug() << "The file doesn't match any supported format:" << filePath;

    return false;
}

void MeshLoader::enableZupToYupConversion(bool b)
{
    mZupToYup = b;
}

bool MeshLoader::loadOgreMeshXML(QString filePath)
{
    QFileInfo info(filePath);

    //static int convertionCount = 0; // just for avoiding name conflicts
    //std::ostringstream sout;
    //sout << "conversion_" << convertionCount++;

    QString meshName = info.completeBaseName();

    Ogre::v1::MeshPtr meshV1Ptr = Ogre::v1::MeshManager::getSingleton().createManual(
        meshName.toStdString(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

    std::string strFilePath = filePath.toStdString();

    Ogre::v1::XMLMeshSerializer xmlMeshSerializer;
    Ogre::VertexElementType colourElementType = Ogre::VET_COLOUR_ABGR;
    xmlMeshSerializer.importMesh(strFilePath, colourElementType, meshV1Ptr.get());

    // Make sure animation types are up to date first
    meshV1Ptr->_determineAnimationTypes();
    meshV1Ptr->buildTangentVectors();

    Ogre::MeshManager& meshMgr = Ogre::MeshManager::getSingleton();

    QString strV2Name = meshName + "_xml";
    Ogre::MeshPtr v2Mesh = meshMgr.createManual(strV2Name.toStdString(), "ViewerResc");
    v2Mesh->importV1(meshV1Ptr.get(), true, true, true);
    Ogre::Item* item = mOgre->sceneManager()->createItem(v2Mesh);

    attachMeshToSceneTree(item);

    return true;
}

bool MeshLoader::loadOgreMesh(QString filePath)
{
    QFileInfo info(filePath);

    std::string sNewResourceLocation = info.absolutePath().toStdString();
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(sNewResourceLocation, "FileSystem", "ViewerResc");

    QString sNewMeshFile = info.fileName();

    Ogre::Item* item = loadOgreV2(sNewMeshFile);
    if (item == nullptr)
    {
        item = loadOgreV1(sNewMeshFile);
    }

    if (item == nullptr)
        return false;

    if (item->getName().empty())
    {
        item->setName(sNewMeshFile.toStdString());
    }

    attachMeshToSceneTree(item);
    return true;
}

bool MeshLoader::loadGLTF(QString filePath)
{
    auto gltf = Ogre_glTF::gltfPluginAccessor::findPlugin()->getLoader();

    Ogre::SceneNode* node = gltf->createScene(filePath.toStdString(),
                                              Ogre_glTF::glTFLoaderInterface::LoadFrom::FileSystem,
                                              mOgre->sceneManager(),
                                              mOgre->meshRootNode());
    if (node)
    {
        QFileInfo info(filePath);
        node->setName(info.fileName().toStdString());
        emit sceneNodeAdded(node);
        return true;
    }
    return false;
}

bool MeshLoader::loadWavefrontObj(QString filePath)
{
    QFileInfo info(filePath);
    QString sOutFile = info.absolutePath() + "/" + info.baseName() + ".mesh";

    ObjImporter objImporter;
    objImporter.setZUpToYUp(mZupToYup);
    Ogre::MeshPtr mesh = objImporter.import(filePath);

    if (!mesh)
        qDebug() << "Failed to import obj:" << filePath;

    Ogre::Item* item = mOgre->sceneManager()->createItem(mesh);
    item->setName(mesh->getName());
    attachMeshToSceneTree(item);

    //return loadOgreMesh(sOutFile);
    return true;
}

Ogre::Item* MeshLoader::loadOgreV1(QString meshName)
{
    Ogre::Item* item = nullptr;
    Ogre::String sMeshName = meshName.toStdString();

    try
    {
        Ogre::v1::MeshManager& meshV1Mgr = Ogre::v1::MeshManager::getSingleton();
        Ogre::v1::MeshPtr v1Mesh = meshV1Mgr.load(sMeshName, "ViewerResc",
                                                  Ogre::v1::HardwareBuffer::HBU_STATIC,
                                                  Ogre::v1::HardwareBuffer::HBU_STATIC);

        Ogre::MeshManager& meshMgr = Ogre::MeshManager::getSingleton();

        QString strV2Name = meshName + " (v1)";
        Ogre::MeshPtr v2Mesh = meshMgr.createManual(strV2Name.toStdString(), "ViewerResc");
        v2Mesh->importV1(v1Mesh.get(), true, true, true);
        item = mOgre->sceneManager()->createItem(v2Mesh);
        item->setName(v2Mesh->getName());
    }
    catch (Ogre::Exception& e) {}
    return item;
}

Ogre::Item* MeshLoader::loadOgreV2(QString meshName)
{
    Ogre::Item* item = nullptr;
    try
    {
        Ogre::MeshManager& meshMgr = Ogre::MeshManager::getSingleton();
        Ogre::MeshPtr mesh = meshMgr.create(meshName.toStdString(), "ViewerResc");
        item = mOgre->sceneManager()->createItem(mesh);
    }
    catch (Ogre::Exception& e) {}
    return item;
}

void MeshLoader::attachMeshToSceneTree(Ogre::Item* item)
{
    auto meshRootNode = mOgre->meshRootNode();
    auto node = meshRootNode->createChildSceneNode();
    node->attachObject(item);
    node->setName(item->getName());
    //qDebug() << "Item Name=" << item->getName().c_str();

    //Ogre::WireAabb* wireAabb = mOgre->sceneManager()->createWireAabb();
    //wireAabb->track(item);

    Ogre::HlmsManager* hlmsMgr = mOgre->ogreRoot()->getHlmsManager();
    Ogre::HlmsTextureManager* hlmsTextureManager = hlmsMgr->getTextureManager();

    for (int i = 0; i < item->getNumSubItems(); ++i)
    {
        auto datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(i)->getDatablock());
        if (datablock == nullptr)
            continue;

        if (datablock == hlmsMgr->getDefaultDatablock())
        {
            item->getSubItem(i)->setDatablock("viewer_default_mtl");
        }

        if (datablock->getTexture(Ogre::PBSM_REFLECTION).isNull())
        {
            auto envMap = hlmsTextureManager->createOrRetrieveTexture("env.dds", Ogre::HlmsTextureManager::TEXTURE_TYPE_ENV_MAP);
            datablock->setTexture(Ogre::PBSM_REFLECTION, envMap.xIdx, envMap.texture);
        }
        /*
        Ogre::HlmsMacroblock macro;
        macro.mPolygonMode = Ogre::PM_WIREFRAME;
        datablock->setMacroblock(macro);
        */
    }

    emit sceneNodeAdded(node);
}
