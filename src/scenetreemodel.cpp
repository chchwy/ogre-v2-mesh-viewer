#include "scenetreemodel.h"

SceneTreeModel::SceneTreeModel(QObject* parent) : QAbstractItemModel(parent)
{
}

QVariant SceneTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // FIXME: Implement me!
    return QVariant();
}

QModelIndex SceneTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    // FIXME: Implement me!
    return QModelIndex();
}

QModelIndex SceneTreeModel::parent(const QModelIndex& index) const
{
    // FIXME: Implement me!
    return QModelIndex();
}

int SceneTreeModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid())
        return 0;

    // FIXME: Implement me!
    return 0;
}

int SceneTreeModel::columnCount(const QModelIndex& parent) const
{
    if (!parent.isValid())
        return 0;

    // FIXME: Implement me!
    return 0;
}

QVariant SceneTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // FIXME: Implement me!
    return QVariant();
}
