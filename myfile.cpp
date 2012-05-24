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

#include "myfile.h"

MyFile::MyFile(QString name, QUrl url, QDateTime zeit, qint32 size)
    :Structureelement(name, url, fileItem), time(zeit)
{
    this->size = size;
}

MyFile::MyFile(QString name, QUrl url, QString zeit, qint32 size)
    : Structureelement(name, url, fileItem), time(QDateTime::fromString(zeit, Qt::ISODate))
{
    this->size = size;
}

QDateTime MyFile::getTime() const
{
    return time;
}


qint32 MyFile::getSize() const
{
    return size;
}
