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

#ifndef STRUKTURELEMENT_H
#define STRUKTURELEMENT_H
#include <QUrl>
#include <QString>
#include <QtGlobal>
#include <QTreeWidgetItem>
#include <QStandardItem>
#include <QStringBuilder>
#include <QClipboard>
#include <QApplication>
#include <QDateTime>

enum MyItemType
{
    semesterItem    = 1000,
    courseItem      = 1001,
    directoryItem   = 1002,
    fileItem        = 1003
};

enum MyItemDataRole
{
    sizeRole            = 32,
    urlRole             = 33,
    dateRole            = 34,
    includeRole         = 35,
    synchronisedRole    = 36,
    cidRole             = 37
};

enum synchroniseStatus
{
    NOT_SYNCHRONISED = 0,
    SYNCHRONISED     = 1,
    NOW_SYNCHRONISED = 2

};

class Structureelement : public QStandardItem
{
public:
    Structureelement(QString name, QUrl url, MyItemType typeEX);
    Structureelement(QString name, QUrl url, int time, qint32 size, MyItemType typeEX = fileItem);
    Structureelement(QString name, QString cid, MyItemType typeEX);

    int type() const;

    bool operator< (const QStandardItem& other) const;

    void setData(const QVariant &value, int role = Qt::UserRole + 1 );
    QVariant data(int role = Qt::UserRole + 1) const;

protected:
    /// Größe des Elements in Bytes
    qint32      size;

    /// Ob die Datei in den Download eingebunden wird
    bool        included;
    QUrl        url;
    QString     cid;
    QDateTime   time;
    MyItemType  typeEX;
    enum synchroniseStatus  synchronised;

private:
    void chooseIcon();


};

#endif // STRUKTURELEMENT_H
