#ifndef MESHLOADER_H
#define MESHLOADER_H

#include <QObject>

namespace Ogre
{
class Item;
}

class OgreManager;


class MeshLoader : public QObject
{
    Q_OBJECT
public:
    MeshLoader(QObject* parent, OgreManager* ogre);
    ~MeshLoader();

    bool load(QString filePath);

private:
    bool loadOgreMeshXML(QString filePath);
    bool loadOgreMesh(QString filePath);
    bool loadGLTF(QString filePath);
    bool loadWavefrontObj(QString filePath);

    Ogre::Item* loadOgreV1(QString meshName);
    Ogre::Item* loadOgreV2(QString meshName);

    OgreManager* mOgre = nullptr;
};

#endif // MESHLOADER_H