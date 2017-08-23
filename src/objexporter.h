#ifndef OBJEXPORTER_H
#define OBJEXPORTER_H

#include <string>

namespace Ogre
{
    class Mesh;
}


class ObjExporter
{
public:
    ObjExporter();

    bool exportFile(Ogre::Mesh* srcMesh, const std::string& sOutFile);
};

#endif // OBJEXPORTER_H