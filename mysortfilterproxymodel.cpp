#include "mysortfilterproxymodel.h"

MySortFilterProxyModel::MySortFilterProxyModel(QObject *parent)
    :QSortFilterProxyModel(parent)
{
}

void MySortFilterProxyModel::setMaximumSize(const qint32 size)
{
    maxSize = size*1024*1024;
    invalidateFilter();
}

void MySortFilterProxyModel::setMaximumSizeFilter(const bool filter)
{
    maxSizeFilter = filter;
    invalidateFilter();
}

bool MySortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    // Holen des Items
    QStandardItemModel* source = (QStandardItemModel*) sourceModel();
    QModelIndex index = source->index(sourceRow, 0, sourceParent);
    Strukturelement* item = (Strukturelement*)source->itemFromIndex(index);

    if (maxSizeFilter)
        return (source->data(index, sizeRole).toInt() <= maxSize);
    return true;
}
