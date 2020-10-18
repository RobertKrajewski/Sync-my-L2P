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

#ifndef MYSORTFILTERPROXYMODEL_H
#define MYSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include "structureelement.h"

class MySortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
     MySortFilterProxyModel(QObject *parent = nullptr);

public slots:
     QDate filterMinimumDate() const { return minDate; }
     void setFilterMinimumDate(const QDate &date);

     QDate filterMaximumDate() const { return maxDate; }
     void setFilterMaximumDate(const QDate &date);

     void setInRangeDateFilter(const bool filter);

     qint32 filterMaximumSize() const { return maxSize; }
     void setMaximumSize(const int size);

     void setMaximumSizeFilter(const bool filter);

 protected:
     bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
//     bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

 private:
     bool dateInRange(const QDate &date) const;
     bool sizeInRange(const qint32 size) const;

     qint32 maxSize;
     bool   maxSizeFilter;

     QDate minDate;
     QDate maxDate;
     bool dateFilter;
};

#endif // MYSORTFILTERPROXYMODEL_H
