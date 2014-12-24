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

#include "structureelement.h"

Structureelement::Structureelement(QString name, QUrl url, MyItemType typeEX)
    :QStandardItem(name), included(true), url(url), typeEX(typeEX)
{
    synchronised = NOT_SYNCHRONISED;
    size = 0;
    setIcon();
}

Structureelement::Structureelement(QString name, QUrl url, int time, qint32 size, MyItemType typeEX)
    :QStandardItem(name),
     included(true),
     url(url),
     time(QDateTime::fromMSecsSinceEpoch(qint64(1000) * time)),
     size(size),
     typeEX(typeEX)
{
    setIcon();
}

Structureelement::Structureelement(QString name, QString cid, MyItemType typeEX)
    :QStandardItem(name), included(true), url(), typeEX(typeEX), cid(cid)
{
    synchronised = NOT_SYNCHRONISED;
    size = 0;
    setIcon();
}

QVariant Structureelement::data(int role) const
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
    else if (role == cidRole)
    {
        return cid;
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

            statustip.append(" - " % time.toString("ddd dd.MM.yy hh:mm"));

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

void Structureelement::setIcon()
{
    if(typeEX == fileItem)
    {
        QString filename = text();
        // Hinzufügen des endungsabhängigen Icons
        // PDF
        if (filename.contains(QRegExp(".pdf$", Qt::CaseInsensitive)))
        {
           setData(QIcon(":/Icons/Icons/1419140442_file-pdf-128.png"), Qt::DecorationRole);
        }
        // ZIP
        else if (filename.contains(QRegExp(".zip$", Qt::CaseInsensitive)))
        {
            setData(QIcon(":/Icons/Icons/filetype_zip.png"), Qt::DecorationRole);
        }
        // RAR
        else if (filename.contains(QRegExp(".rar$", Qt::CaseInsensitive)))
        {
            setData(QIcon(":/Icons/Icons/filetype_rar.png"), Qt::DecorationRole);
        }
        // Sonstige
        else
        {
            setData(QIcon(":/Icons/Icons/file.png"), Qt::DecorationRole);
        }
    }
    else if(typeEX == courseItem)
    {
        setData(QIcon(":/Icons/Icons/1419123850_paste.png"), Qt::DecorationRole);
    }
    else if(typeEX == directoryItem)
    {
        setData(QIcon(":/Icons/Icons/1419140428_folder-blue-128.png"), Qt::DecorationRole);
    }
    else if(typeEX == semesterItem)
    {
        setData(QIcon(":/Icons/Icons/1419140388_calendar-128.png"), Qt::DecorationRole);
    }

}

bool Structureelement::operator< (const QStandardItem& other) const
{
    if ((this->size == 0) && ((((Structureelement*)(&other))->size) != 0))
        return true;
    else if ((this->size != 0) && ((((Structureelement*)(&other))->size) == 0))
        return false;
    else
        return (this->text().toLower() < (((Structureelement*)(&other))->text()).toLower());
}

int Structureelement::type() const
{
    return typeEX;
}

void Structureelement::setData(const QVariant &value, int role)
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
    else if (role == cidRole)
    {
        this->cid = value.toString();
    }
    else
    {
        QStandardItem::setData(value, role);
    }
}
