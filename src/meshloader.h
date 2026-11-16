#ifndef MESHLOADER_H
#define MESHLOADER_H

#include <QObject>

namespace Ogre
{
class Item;
class SceneNode;
}

class OgreManager;
class TiXmlElement;


class MeshLoader : public QObject
{
    Q_OBJECT
public:
    MeshLoader(QObject* parent, OgreManager* ogre);
    ~MeshLoader();

    bool load(QString filePath);
    void enableZupToYupConversion(bool b);

    /// Loads a .mesh file (v2, or v1 converted to v2) without creating any scene object.
    /// Returns a null pointer when the file cannot be read.
    static Ogre::MeshPtr loadMeshFile(const QString& filePath);

signals:
    void sceneNodeAdded(Ogre::SceneNode* node);

private:
    bool loadOgreMeshXML(QString filePath);
    bool loadOgreMesh(QString filePath);
    bool loadGLTF(QString filePath);
    bool loadWavefrontObj(QString filePath);


    void attachMeshToSceneTree(Ogre::Item* item);
    void applyDefaultMaterials(Ogre::Item* item);
    void loadMaterialsFromFolder(const QString& folder);

    bool loadDotScene(QString filePath);
    void loadDotSceneNode(const TiXmlElement* element, Ogre::SceneNode* parent, const QString& sceneFolder);
    void readDotSceneTransform(const TiXmlElement* element, Ogre::SceneNode* node);

    OgreManager* mOgre = nullptr;
    bool mZupToYup = false;
};

#endif // MESHLOADER_H
