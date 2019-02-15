#ifndef MESHLOADER_H
#define MESHLOADER_H

#include <QObject>

namespace Ogre
{
class Item;
class SceneNode;
}

class OgreManager;


class MeshLoader : public QObject
{
    Q_OBJECT
public:
    MeshLoader(QObject* parent, OgreManager* ogre);
    ~MeshLoader();

    bool load(QString filePath);
    void enableZupToYupConversion(bool b);

signals:
    void sceneNodeAdded(Ogre::SceneNode* node);

private:
    bool loadOgreMeshXML(QString filePath);
    bool loadOgreMesh(QString filePath);
    bool loadGLTF(QString filePath);
    bool loadWavefrontObj(QString filePath);

    Ogre::Item* loadOgreV1(QString meshName);
    Ogre::Item* loadOgreV2(QString meshName);

    void attachMeshToSceneTree(Ogre::Item* item);

    OgreManager* mOgre = nullptr;
    bool mZupToYup = false;
};

#endif // MESHLOADER_H
