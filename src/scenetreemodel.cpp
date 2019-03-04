#include "scenetreemodel.h"

#include "OgreSceneNode.h"
#include "OgreSceneManager.h"

#include "ogremanager.h"

SceneTreeModel::SceneTreeModel(QObject* parent, OgreManager* ogre) : QAbstractItemModel(parent)
{
    mOgre = ogre;
}

SceneTreeModel::~SceneTreeModel()
{
    mRootNode = nullptr;
}

QVariant SceneTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section == 0)
        return QString("Node");

    //if (section == 1)
    //    return QString("Summary");

    return QVariant();
}

QModelIndex SceneTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    Ogre::SceneNode* parentNode = nullptr;
    if (!parent.isValid())
    {
        parentNode = mRootNode;
    }
    else
    {
        parentNode = static_cast<Ogre::SceneNode*>(parent.internalPointer());
    }

    Ogre::SceneNode* node = static_cast<Ogre::SceneNode*>(parentNode->getChild(row));
    if (node)
    {
        //qDebug() << "create node!";
        return createIndex(row, column, (void*)node);
    }

    return QModelIndex();
}

QModelIndex SceneTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    auto node = static_cast<Ogre::SceneNode*>(index.internalPointer());
    auto parentNode = node->getParentSceneNode();

    if (parentNode == mRootNode)
        return QModelIndex();

    //qDebug() << "create parent index";
    return createIndex(getSceneNodeRow(parentNode), 0, (void*)parentNode);
}

int SceneTreeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0)
        return 0;

    Ogre::SceneNode* parentNode = nullptr;
    if (!parent.isValid())
        parentNode = mRootNode;
    else
        parentNode = static_cast<Ogre::SceneNode*>(parent.internalPointer());

    //qDebug() << "RowCount " << QString::fromStdString(parentNode->getName()) << "=" << parentNode->numChildren();
    return parentNode->numChildren();
}

int SceneTreeModel::columnCount(const QModelIndex& parent) const
{
    return 1;
}

QVariant SceneTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    Ogre::SceneNode* node = static_cast<Ogre::SceneNode*>(index.internalPointer());
    return QVariant(QString::fromStdString(node->getName()));
}

Qt::ItemFlags SceneTreeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return 0;

    return /*Qt::ItemIsEditable |*/ QAbstractItemModel::flags(index);
}

bool SceneTreeModel::setData(const QModelIndex& index, const QVariant &value, int role /*= Qt::EditRole*/)
{
    if (role != Qt::EditRole)
        return false;

    //TreeItem *item = getItem(index);
    //bool result = item->setData(index.column(), value);
    bool result = false;
    if (result)
        emit dataChanged(index, index);

    return result;
}

bool SceneTreeModel::setHeaderData(int, Qt::Orientation, const QVariant&, int)
{
    return false;
}

bool SceneTreeModel::insertRows(int position, int rows, const QModelIndex& parent)
{
    //qDebug() << "InsertRows pos:" << position << ", rows:" << rows << ", index:" << parent;

    Ogre::SceneNode* parentNode = nullptr;
    if (!parent.isValid())
        parentNode = mRootNode;
    else
        parentNode = static_cast<Ogre::SceneNode*>(parent.internalPointer());

    bool success = false;

    beginInsertRows(parent, position, position + rows - 1);
    //success = parentItem->insertChildren(position, rows, rootItem->columnCount());
    endInsertRows();

    return true;
}

bool SceneTreeModel::removeRows(int position, int rows, const QModelIndex& parent /*= QModelIndex()*/)
{
    return false;
}

void SceneTreeModel::refresh(QModelIndex rootIndex)
{
    mRootNode = mOgre->meshRootNode();

    beginInsertRows(rootIndex, 0, 1);
    endInsertRows();
}

int SceneTreeModel::getSceneNodeRow(Ogre::SceneNode* node) const
{
    if (node == mRootNode)
        return 0;

    Ogre::SceneNode* parentNode = node->getParentSceneNode();
    if (parentNode == nullptr)
        return 0;

    int index = 0;
    for (int i = 0; i < parentNode->numChildren(); ++i)
    {
        if (parentNode->getChild(i) == node)
        {
            return i;
        }
    }
    Q_ASSERT(false);
    return 0;
}
