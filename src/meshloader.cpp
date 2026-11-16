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
#include <QDir>

#include "tinyxml.h"

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
    else if (filePath.endsWith(".scene"))
    {
        return loadDotScene(filePath);
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

    loadMaterialsFromFolder(info.absolutePath());

    Ogre::MeshPtr mesh = loadMeshFile(filePath);
    if (!mesh)
        return false;

    Ogre::Item* item = mOgre->sceneManager()->createItem(mesh);
    item->setName(mesh->getName());

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

Ogre::MeshPtr MeshLoader::loadMeshFile(const QString& filePath)
{
    const QFileInfo info(filePath);
    const std::string folder = info.absolutePath().toStdString();
    const std::string fileName = info.fileName().toStdString();

    auto& resourceMgr = Ogre::ResourceGroupManager::getSingleton();
    if (!resourceMgr.resourceLocationExists(folder, "ViewerResc"))
        resourceMgr.addResourceLocation(folder, "FileSystem", "ViewerResc");

    // A v1 file that was converted earlier (e.g. the same mesh used twice in a scene)
    const std::string v1ImportName = fileName + " (v1 import)";
    if (Ogre::MeshPtr imported = Ogre::MeshManager::getSingleton().getByName(v1ImportName, "ViewerResc"))
        return imported;

    try
    {
        return Ogre::MeshManager::getSingleton().load(fileName, "ViewerResc");
    }
    catch (Ogre::Exception&) {}

    try
    {
        Ogre::v1::MeshManager& meshV1Mgr = Ogre::v1::MeshManager::getSingleton();
        Ogre::v1::MeshPtr v1Mesh = meshV1Mgr.load(fileName, "ViewerResc",
                                                  Ogre::v1::HardwareBuffer::HBU_STATIC,
                                                  Ogre::v1::HardwareBuffer::HBU_STATIC);

        Ogre::MeshPtr v2Mesh = Ogre::MeshManager::getSingleton().createManual(v1ImportName, "ViewerResc");
        v2Mesh->importV1(v1Mesh.get(), true, true, true);
        meshV1Mgr.remove(v1Mesh->getHandle());
        return v2Mesh;
    }
    catch (Ogre::Exception& e)
    {
        qDebug() << "Cannot load mesh" << filePath << e.what();
    }
    return Ogre::MeshPtr();
}

void MeshLoader::attachMeshToSceneTree(Ogre::Item* item)
{
    auto meshRootNode = mOgre->meshRootNode();
    auto node = meshRootNode->createChildSceneNode();
    node->attachObject(item);
    node->setName(item->getName());

    applyDefaultMaterials(item);

    emit sceneNodeAdded(node);
}

void MeshLoader::applyDefaultMaterials(Ogre::Item* item)
{
    Ogre::HlmsManager* hlmsMgr = mOgre->ogreRoot()->getHlmsManager();

    for (int i = 0; i < item->getNumSubItems(); ++i)
    {
        auto datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(i)->getDatablock());
        if (datablock == nullptr)
            continue;

        if (datablock == hlmsMgr->getDefaultDatablock())
        {
            Ogre::HlmsPbs* hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlmsMgr->getHlms(Ogre::HLMS_PBS) );

            // Imported files (e.g. a re-imported .mtl) may already use a TmpBlock name
            static int cnt = 0;
            std::stringstream sout;
            do
            {
                sout.str("");
                sout << "TmpBlock_" << cnt;
                cnt++;
            } while (hlmsPbs->getDatablock(sout.str()) != nullptr);

            Ogre::HlmsBlendblock blend;
            blend.mSourceBlendFactorAlpha = Ogre::SBF_SOURCE_ALPHA;
            blend.mDestBlendFactorAlpha = Ogre::SBF_ONE_MINUS_SOURCE_ALPHA;

            Ogre::HlmsPbsDatablock* datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                hlmsPbs->createDatablock(sout.str(),
                                         sout.str(),
                                         Ogre::HlmsMacroblock(),
                                         blend,
                                         Ogre::HlmsParamVec()));

            datablock->setWorkflow(Ogre::HlmsPbsDatablock::MetallicWorkflow);
            datablock->setRoughness(0.7);
            datablock->setMetalness(0.3);
            datablock->setTexture(Ogre::PBSM_REFLECTION, "env.dds");


            item->getSubItem(i)->setDatablock(datablock);

        }

        if (datablock->getTexture(Ogre::PBSM_REFLECTION) == nullptr)
        {
            datablock->setTexture(Ogre::PBSM_REFLECTION, "env.dds");

        }
    }
}

void MeshLoader::loadMaterialsFromFolder(const QString& folder)
{
    const std::string location = folder.toStdString();
    auto& manager = Ogre::ResourceGroupManager::getSingleton();
    if (manager.resourceLocationExists(location, "ViewerResc"))
        return;

    manager.addResourceLocation(location, "FileSystem", "ViewerResc");
    auto allMaterials = manager.findResourceNames("ViewerResc", "*.material.json");
    for (const std::string& sMtlName : *allMaterials)
    {
        Ogre::Root::getSingleton().getHlmsManager()->loadMaterials(sMtlName, "ViewerResc", nullptr, "");
    }
}

namespace
{
Ogre::Real realAttribute(const TiXmlElement* element, const char* name, Ogre::Real defaultValue)
{
    double value = defaultValue;
    if (element && element->QueryDoubleAttribute(name, &value) == TIXML_SUCCESS)
        return Ogre::Real(value);
    return defaultValue;
}
}

bool MeshLoader::loadDotScene(QString filePath)
{
    TiXmlDocument document;
    if (!document.LoadFile(filePath.toLocal8Bit().constData()))
    {
        qDebug() << "Cannot parse scene file:" << filePath << document.ErrorDesc();
        return false;
    }

    const TiXmlElement* sceneElement = document.FirstChildElement("scene");
    const TiXmlElement* nodesElement = sceneElement ? sceneElement->FirstChildElement("nodes") : nullptr;
    if (nodesElement == nullptr)
    {
        qDebug() << "No <scene><nodes> in" << filePath;
        return false;
    }

    const QFileInfo info(filePath);
    loadMaterialsFromFolder(info.absolutePath());

    Ogre::SceneNode* sceneRoot = mOgre->meshRootNode()->createChildSceneNode();
    sceneRoot->setName(info.fileName().toStdString());

    // The <nodes> element may carry a transform of its own
    readDotSceneTransform(nodesElement, sceneRoot);

    for (const TiXmlElement* child = nodesElement->FirstChildElement("node"); child; child = child->NextSiblingElement("node"))
        loadDotSceneNode(child, sceneRoot, info.absolutePath());

    emit sceneNodeAdded(sceneRoot);
    return true;
}

void MeshLoader::readDotSceneTransform(const TiXmlElement* element, Ogre::SceneNode* node)
{
    if (const TiXmlElement* position = element->FirstChildElement("position"))
    {
        node->setPosition(realAttribute(position, "x", 0), realAttribute(position, "y", 0), realAttribute(position, "z", 0));
    }

    // Exporters disagree on the element and attribute names of the orientation
    const TiXmlElement* rotation = element->FirstChildElement("rotation");
    if (rotation == nullptr)
        rotation = element->FirstChildElement("quaternion");
    if (rotation)
    {
        Ogre::Quaternion q = Ogre::Quaternion::IDENTITY;
        if (rotation->Attribute("qw"))
        {
            q = Ogre::Quaternion(realAttribute(rotation, "qw", 1), realAttribute(rotation, "qx", 0),
                                 realAttribute(rotation, "qy", 0), realAttribute(rotation, "qz", 0));
        }
        else if (rotation->Attribute("w"))
        {
            q = Ogre::Quaternion(realAttribute(rotation, "w", 1), realAttribute(rotation, "x", 0),
                                 realAttribute(rotation, "y", 0), realAttribute(rotation, "z", 0));
        }
        else if (rotation->Attribute("angle"))
        {
            const Ogre::Vector3 axis(realAttribute(rotation, "axisX", 0), realAttribute(rotation, "axisY", 1), realAttribute(rotation, "axisZ", 0));
            q.FromAngleAxis(Ogre::Radian(realAttribute(rotation, "angle", 0)), axis.normalisedCopy());
        }
        q.normalise();
        node->setOrientation(q);
    }

    if (const TiXmlElement* scale = element->FirstChildElement("scale"))
    {
        node->setScale(realAttribute(scale, "x", 1), realAttribute(scale, "y", 1), realAttribute(scale, "z", 1));
    }
}

void MeshLoader::loadDotSceneNode(const TiXmlElement* element, Ogre::SceneNode* parent, const QString& sceneFolder)
{
    Ogre::SceneNode* node = parent->createChildSceneNode();
    if (const char* name = element->Attribute("name"))
        node->setName(name);

    readDotSceneTransform(element, node);

    for (const TiXmlElement* entity = element->FirstChildElement("entity"); entity; entity = entity->NextSiblingElement("entity"))
    {
        const char* meshFile = entity->Attribute("meshFile");
        if (meshFile == nullptr)
            continue;

        Ogre::MeshPtr mesh = loadMeshFile(QDir(sceneFolder).filePath(QString::fromUtf8(meshFile)));
        if (!mesh)
        {
            qDebug() << "Scene refers to a mesh that cannot be loaded:" << meshFile;
            continue;
        }

        Ogre::Item* item = mOgre->sceneManager()->createItem(mesh);
        item->setName(entity->Attribute("name") ? entity->Attribute("name") : meshFile);
        node->attachObject(item);

        Ogre::HlmsManager* hlmsMgr = mOgre->ogreRoot()->getHlmsManager();

        // A material on the entity applies to every sub item, <subentity> overrides one of them
        if (const char* material = entity->Attribute("material"))
        {
            if (Ogre::HlmsDatablock* datablock = hlmsMgr->getDatablockNoDefault(material))
                item->setDatablock(datablock);
        }
        if (const TiXmlElement* subEntities = entity->FirstChildElement("subentities"))
        {
            for (const TiXmlElement* sub = subEntities->FirstChildElement("subentity"); sub; sub = sub->NextSiblingElement("subentity"))
            {
                int index = -1;
                const char* material = sub->Attribute("materialName");
                if (sub->QueryIntAttribute("index", &index) != TIXML_SUCCESS || material == nullptr)
                    continue;

                Ogre::HlmsDatablock* datablock = hlmsMgr->getDatablockNoDefault(material);
                if (datablock && index >= 0 && index < int(item->getNumSubItems()))
                    item->getSubItem(size_t(index))->setDatablock(datablock);
            }
        }

        applyDefaultMaterials(item);
    }

    // Lights and cameras are not shown by the viewer; only the node hierarchy and meshes are kept
    for (const TiXmlElement* child = element->FirstChildElement("node"); child; child = child->NextSiblingElement("node"))
        loadDotSceneNode(child, node, sceneFolder);
}