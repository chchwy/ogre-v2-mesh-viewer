#include "stdafx.h"
#include "meshloader.h"

// ogre headers
#include <OgreItem.h>
#include <OgreHlmsPbsDatablock.h>
#include <OgreMeshManager2.h>
#include <OgreMeshManager.h>
#include <OgreMesh2.h>

// qt headers
#include <QFile>
#include <QFileInfo>

#include "ogremanager.h"
#include "objimporter.h"



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

bool MeshLoader::loadOgreMeshXML(QString filePath)
{
    return true;
}

bool MeshLoader::loadOgreMesh(QString filePath)
{
     QFileInfo info(filePath);

    std::string sNewResourceLocation = info.absolutePath().toStdString();
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(sNewResourceLocation, "FileSystem", "OgreSpooky");

    QString sNewMeshFile = info.fileName();

    Ogre::Item* pItem = loadOgreV2(sNewMeshFile);
    if (pItem == nullptr)
    {
        pItem = loadOgreV1(sNewMeshFile);
    }

    if (pItem == nullptr)
        return false;

    auto meshRootNode = mOgre->meshRootNode();
    auto node = meshRootNode->createChildSceneNode();
    node->attachObject(pItem);

    Ogre::HlmsManager* hlmsMgr = mOgre->ogreRoot()->getHlmsManager();
    for (int i = 0; i < pItem->getNumSubItems(); ++i)
    {
        auto datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(pItem->getSubItem(i)->getDatablock());
        if (datablock == nullptr)
        {
            continue;
        }

        if (datablock == hlmsMgr->getDefaultDatablock())
        {
            pItem->getSubItem(i)->setDatablock("viewer_default_mtl");
        }

        if (datablock->getTexture(Ogre::PBSM_REFLECTION).isNull())
        {
            auto hlmsTextureManager = hlmsMgr->getTextureManager();
            auto envMap = hlmsTextureManager->createOrRetrieveTexture("env.dds", Ogre::HlmsTextureManager::TEXTURE_TYPE_ENV_MAP);
            datablock->setTexture(Ogre::PBSM_REFLECTION, envMap.xIdx, envMap.texture);
        }
        /*
        Ogre::HlmsMacroblock macro;
        macro.mPolygonMode = Ogre::PM_WIREFRAME;
        datablock->setMacroblock(macro);
        */
    }
    return true;
    return true;
}

bool MeshLoader::loadGLTF(QString filePath)
{
    return true;
}

bool MeshLoader::loadWavefrontObj(QString filePath)
{
    QFileInfo info(filePath);
    QString sOutFile = info.absolutePath() + "/" + info.baseName() + ".mesh";

    ObjImporter objImporter;
    //objImporter.setZUpToYUp(ret == QMessageBox::Yes);
    bool ok = objImporter.import(filePath, sOutFile);
    //qDebug() << "Obj=" << filePath << ", Success=" << ok;

    if (!ok)
    {
        qDebug() << "Failed to import obj:" << filePath;
    }
    return loadOgreMesh(sOutFile);
}

Ogre::Item* MeshLoader::loadOgreV1(QString meshName)
{
    Ogre::Item* item = nullptr;
    Ogre::String sMeshName = meshName.toStdString();

    try
    {
        Ogre::v1::MeshManager& meshV1Mgr = Ogre::v1::MeshManager::getSingleton();
        Ogre::v1::MeshPtr v1Mesh = meshV1Mgr.load(sMeshName, "OgreSpooky",
                                                  Ogre::v1::HardwareBuffer::HBU_STATIC,
                                                  Ogre::v1::HardwareBuffer::HBU_STATIC);

        Ogre::MeshManager& meshMgr = Ogre::MeshManager::getSingleton();

        QString strV2Name = meshName + "_v2import";
        Ogre::MeshPtr v2Mesh = meshMgr.createManual(strV2Name.toStdString(), "OgreSpooky");
        v2Mesh->importV1(v1Mesh.get(), true, true, true);
        item = mOgre->sceneManager()->createItem(v2Mesh);
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
        Ogre::MeshPtr mesh = meshMgr.create(meshName.toStdString(), "OgreSpooky");
        item = mOgre->sceneManager()->createItem(mesh);
    }
    catch(Ogre::Exception& e) {}
    return item;
}
