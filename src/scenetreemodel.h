#pragma once

#include <QAbstractItemModel>

namespace Ogre
{
class SceneNode;
}
class OgreManager;


class SceneTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit SceneTreeModel(QObject* parent, OgreManager* ogre);
    ~SceneTreeModel();

    // Basic functionality from QAbstractItemModel
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    void refresh();

private:
    int getSceneNodeRow(Ogre::SceneNode*) const;

    Ogre::SceneNode* mRootNode = nullptr;
    OgreManager* mOgre = nullptr;
};
