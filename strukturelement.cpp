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

#include "strukturelement.h"
#include "datei.h"

Strukturelement::Strukturelement(QString name, QUrl url, MyItemType type)
    :QStandardItem(name), included(true), url(url), typeEX(type)
{
    synchronised = NOT_SYNCHRONISED;
    size = 0;
}

QVariant Strukturelement::data(int role) const
{
    if (role == includeRole)
    {
        return included;
    }
    else if (role == urlRole)
    {
        return url;
    }
    else if (role == sizeRole)
    {
        return size;
    }
    else if (role == dateRole)
    {
        return time;
    }
    else if (role == synchronisedRole)
    {
        return synchronised;
    }
    else if (role == Qt::StatusTipRole)
    {
        QString statustip;

        if (typeEX == fileItem)
        {
            statustip.append(text() % " - ");
            if (size > 1048576)
                statustip.append(QString::number(size/1048576.0,'f',2) % " MB");
            else if(size > 1024)
                 statustip.append(QString::number(size/1024.0,'f',2) % " KB");
            else
                 statustip.append(QString::number(size) % " Byte");

            statustip.append(" - " % ((Datei*)this)->GetTime().toString("ddd dd.MM.yy hh:mm"));

            return statustip;
        }
        return statustip;
    }
    else if (role == Qt::FontRole)
    {
        if (typeEX == courseItem)
        {
            QFont BoldFont;
            BoldFont.setBold(true);
            return BoldFont;
        }
    }
    else if(role == Qt::ForegroundRole)
    {
        if (included)
            if (synchronised == NOW_SYNCHRONISED)
                return QBrush(Qt::blue);
            else if (synchronised == SYNCHRONISED)
                return QBrush(Qt::darkGreen);
            else
                return QBrush(Qt::black);
        else
            return QBrush(Qt::red);
    }
    return QStandardItem::data(role);
}

bool Strukturelement::operator< (const QStandardItem& other) const
{
    if ((this->size == 0) && ((((Strukturelement*)(&other))->size) != 0))
        return true;
    else if ((this->size != 0) && ((((Strukturelement*)(&other))->size) == 0))
        return false;
    else
        return (this->text().toLower() < (((Strukturelement*)(&other))->text()).toLower());
}

int Strukturelement::type() const
{
    return typeEX;
}

void Strukturelement::setData(const QVariant &value, int role)
{
    if (role == includeRole)
    {
        this->included = value.toBool();
    }
    else if (role == urlRole)
    {
        this->url = value.toUrl();
    }
    else if (role == sizeRole)
    {
        this->size = value.toInt();
    }
    else if (role == dateRole)
    {
        this->time = value.toDateTime();
    }
    else if (role == synchronisedRole)
    {
        this->synchronised = (synchroniseStatus) value.toInt();
    }
    else
    {
        QStandardItem::setData(value, role);
    }
}
