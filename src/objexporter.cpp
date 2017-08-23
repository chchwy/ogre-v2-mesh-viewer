
#include "stdafx.h"
#include "objexporter.h"
#include "OgreXMLMeshSerializer.h"

#include "OgreMesh.h"
#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"


ObjExporter::ObjExporter()
{

}

bool ObjExporter::exportFile(Ogre::Mesh* srcMesh, const std::string& sOutFile)
{
    Q_ASSERT(srcMesh);
    auto& meshV1Mgr = Ogre::v1::MeshManager::getSingleton();

    static int convertionCount = 4431; // just to avoid name conflicts
    std::ostringstream sout;
    sout << "conversion_" << convertionCount++;

    Ogre::v1::MeshPtr v1Mesh = meshV1Mgr.createManual(sout.str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    v1Mesh->importV2(srcMesh);

    Ogre::v1::XMLMeshSerializer xmlSerializer;
    xmlSerializer.exportMesh(v1Mesh.get(), "C:/Users/Matt/Desktop/abc.mesh.xml");

    

    return true;
}
