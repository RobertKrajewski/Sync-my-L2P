/****************************************************************************
** This file is part of Sync-my-L2P.
**
** Sync-my-L2P is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Sync-my-L2P is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with Sync-my-L2P.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#include "mysortfilterproxymodel.h"
#include <QDebug>

MySortFilterProxyModel::MySortFilterProxyModel(QObject *parent)
    :QSortFilterProxyModel(parent)
{
    // Standardwerte hier müssen mit Standardwerten in GUI übereinstimmen,
    // da es sonst Probleme mit den valueChanged Signals gibt

    maxSizeFilter = false;
    maxSize = 1024*1024;

    dateFilter = false;
    minDate = QDate(1, 1, 1);
    maxDate = QDate(1, 1, 1);
}

void MySortFilterProxyModel::setFilterMinimumDate(const QDate &date)
{
    minDate = date;
    invalidateFilter();
}

void MySortFilterProxyModel::setFilterMaximumDate(const QDate &date)
{
    maxDate = date;
    invalidateFilter();
}

void MySortFilterProxyModel::setInRangeDateFilter(const bool filter)
{
    dateFilter = filter;
    invalidateFilter();
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

    // Prüfen der Filterbedingungen
    bool show = true;
    if (maxSizeFilter)
        show = show && sizeInRange(source->data(index, sizeRole).toInt());
    if (dateFilter && source->itemFromIndex(index)->type() == fileItem)
        show = show && dateInRange(source->data(index, dateRole).toDate());
    return show;
}

bool MySortFilterProxyModel::dateInRange(const QDate &date) const
{
    return (((date <= maxDate) || !maxDate.isValid()) && ((date >= minDate) || !minDate.isValid()));
}

bool MySortFilterProxyModel::sizeInRange(const qint32 size) const
{
    return (size <= maxSize);
}
