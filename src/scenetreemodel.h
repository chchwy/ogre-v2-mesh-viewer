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

    // QTreeView 
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    // QTreeView editing
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant &value,
                 int role = Qt::EditRole) override;
    bool setHeaderData(int section, Qt::Orientation orientation,
                       const QVariant &value, int role = Qt::EditRole) override;
    bool insertRows(int position, int rows,
                    const QModelIndex& parent = QModelIndex()) override;
    bool removeRows(int position, int rows,
                    const QModelIndex& parent = QModelIndex()) override;

    void refresh();

private:
    int getSceneNodeRow(Ogre::SceneNode*) const;

    Ogre::SceneNode* mRootNode = nullptr;
    OgreManager* mOgre = nullptr;
};
