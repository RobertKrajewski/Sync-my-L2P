#ifndef MYSORTFILTERPROXYMODEL_H
#define MYSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <strukturelement.h>

class MySortFilterProxyModel : public QSortFilterProxyModel
{
public:
     MySortFilterProxyModel(QObject *parent = 0);

//     QDate filterMinimumDate() const { return minDate; }
//     void setFilterMinimumDate(const QDate &date);

//     QDate filterMaximumDate() const { return maxDate; }
//     void setFilterMaximumDate(const QDate &date);

     qint32 filterMaximumSize() const { return maxSize; }
     void setMaximumSize(const qint32 size);

     void setMaximumSizeFilter(const bool filter);

 protected:
     bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
//     bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

 private:
//     bool dateInRange(const QDate &date) const;
     bool sizeInRange(const qint32 size) const;

     qint32 maxSize;
     bool maxSizeFilter;

//     QDate minDate;
//     QDate maxDate;
};

#endif // MYSORTFILTERPROXYMODEL_H
