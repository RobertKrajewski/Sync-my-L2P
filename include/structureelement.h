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
#include <QStandardItem>
#include <QDateTime>

enum MyItemSystem
{
    l2p    = 100,
    moodle = 101
};

enum MyItemType
{
    semesterItem    = 1000,
    courseItem      = 1001,
    directoryItem   = 1002,
    fileItem        = 1003,
    messageItem     = 1004
};

enum MyItemDataRole
{
    sizeRole            = 32,
    urlRole             = 33,
    dateRole            = 34,
    includeRole         = 35,
    synchronisedRole    = 36,
    cidRole             = 37,
    bodyRole            = 38,
    topicRole           = 39,
    authorRole          = 40,
    typeEXRole          = 41,
    systemEXRole        = 42
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
    Structureelement(QString name, QUrl url = QUrl(), int time = 0, qint32 size = 0, QString cid = "", MyItemType typeEX = fileItem, MyItemSystem systemEX = l2p);

    Structureelement(QString body, QString topic, QString author, int time = 0, QString cid = "", MyItemType typeEX = messageItem, MyItemSystem systemEX = l2p);

    int type() const { return typeEX; }

    bool operator< (const QStandardItem& other) const;

    void setData(const QVariant &value, int role = Qt::UserRole + 1 );
    QVariant data(int role = Qt::UserRole + 1) const;

protected:
    /// Größe des Elements in Bytes
    qint32      size;

    /// Ob die Datei in den Download eingebunden wird
    bool        included;

    /// Url für die API
    QUrl        url;

    /// Veranstaltungs ID
    QString     cid;

    /// Änderungsdatum
    QDateTime   time;

    /// Inhalt der Nachricht
    QString body;

    /// Thema der Nachricht
    QString topic;

    /// Autor der Nachricht
    QString author;

    /// Type des Elements
    MyItemType  typeEX;

    /// System des Elements
    MyItemSystem  systemEX;

    /// Status der Synchronisation
    enum synchroniseStatus  synchronised;

    // Neuer Bereich für die Nachrichten

private:
    void chooseIcon();
};

#endif // STRUKTURELEMENT_H
