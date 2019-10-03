#include "stdafx.h"

#include "meshloader.h"

// ogre headers
#include <OgreItem.h>
#include <OgreHlmsPbsDatablock.h>
#include <OgreMeshManager2.h>
#include <OgreMeshManager.h>
#include <OgreMesh2.h>
#include <OgreWireAabb.h>
#include <OgreHlmsPbs.h>

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
    v2Mesh->importV1(meshV1Ptr.get(), false, false, true);

    Ogre::Item* item = mOgre->sceneManager()->createItem(v2Mesh);
    item->setName(strV2Name.toStdString());

    attachMeshToSceneTree(item);

    // remove the temp v1 mesh
    //Ogre::v1::MeshManager::getSingleton().remove(meshV1Ptr->getHandle());

    return true;
}

bool MeshLoader::loadOgreMesh(QString filePath)
{
    QFileInfo info(filePath);

    std::string sNewResourceLocation = info.absolutePath().toStdString();
    auto& manager = Ogre::ResourceGroupManager::getSingleton();
    if (!manager.resourceLocationExists(sNewResourceLocation, "ViewerResc"))
    {
        manager.addResourceLocation(sNewResourceLocation, "FileSystem", "ViewerResc");
        auto allMaterials = manager.findResourceNames("ViewerResc", "*.material.json");
        for (const std::string& sMtlName : *allMaterials)
        {
            Ogre::Root::getSingleton().getHlmsManager()->loadMaterials(sMtlName, "ViewerResc", nullptr, "");
        }
    }

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
    Ogre::MeshPtr mesh = objImporter.import(filePath, true);

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

        meshV1Mgr.remove(v1Mesh->getHandle());
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

    //Ogre::WireAabb* wireAabb = mOgre->sceneManager()->createWireAabb();
    //wireAabb->track(item);

    Ogre::HlmsManager* hlmsMgr = mOgre->ogreRoot()->getHlmsManager();
    //Ogre::HlmsTextureManager* hlmsTextureManager = hlmsMgr->getTextureManager();

    for (int i = 0; i < item->getNumSubItems(); ++i)
    {
        auto datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(i)->getDatablock());
        if (datablock == nullptr)
            continue;

        if (datablock == hlmsMgr->getDefaultDatablock())
        {
            static int cnt = 0;
            std::stringstream sout;
            sout << "TmpBlock_" << cnt;
            cnt++;

            Ogre::HlmsBlendblock blend;
            blend.mSourceBlendFactorAlpha = Ogre::SBF_SOURCE_ALPHA;
            blend.mDestBlendFactorAlpha = Ogre::SBF_ONE_MINUS_SOURCE_ALPHA;

            Ogre::HlmsPbs* hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlmsMgr->getHlms(Ogre::HLMS_PBS) );
            Ogre::HlmsPbsDatablock* datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                hlmsPbs->createDatablock(sout.str(),
                                         sout.str(),
                                         Ogre::HlmsMacroblock(),
                                         blend,
                                         Ogre::HlmsParamVec()));

            datablock->setWorkflow(Ogre::HlmsPbsDatablock::MetallicWorkflow);
            datablock->setRoughness(0.7);
            datablock->setMetalness(0.3);
            //auto envMap = hlmsTextureManager->createOrRetrieveTexture("env.dds", Ogre::HlmsTextureManager::TEXTURE_TYPE_ENV_MAP);
            //datablock->setTexture(Ogre::PBSM_REFLECTION, envMap.xIdx, envMap.texture);

            item->getSubItem(i)->setDatablock(datablock);

        }
        /*
        if (datablock->getTexture(Ogre::PBSM_REFLECTION).isNull())
        {
            auto envMap = hlmsTextureManager->createOrRetrieveTexture("env.dds", Ogre::HlmsTextureManager::TEXTURE_TYPE_ENV_MAP);
            datablock->setTexture(Ogre::PBSM_REFLECTION, envMap.xIdx, envMap.texture);
        }
        */
    }

    emit sceneNodeAdded(node);
}
