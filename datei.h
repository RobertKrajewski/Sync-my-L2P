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

#ifndef DATEI_H
#define DATEI_H
#include <strukturelement.h>


class Datei : public Strukturelement
{
public:
    Datei(QString name, QUrl url, QDateTime time, qint32 size);
    Datei(QString name, QUrl url, QString time, qint32 size);
    QDateTime GetTime() const;
    qint32 getSize() const;

private:
    QDateTime time;
};

#endif // DATEI_H
