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
    setCheckable(true);
    setTristate(true);
}

Structureelement::Structureelement(QString name, QUrl url, QString time, qint32 size, MyItemType typeEX)
    :QStandardItem(name), included(false), url(url),time(QDateTime::fromString(time, Qt::ISODate)), size(size), typeEX(typeEX)
{
    setCheckable(true);
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
    else if (role == Qt::CheckStateRole)
    {
        if (typeEX == fileItem)
        {
            return included && synchronised == NOT_SYNCHRONISED ? Qt::Checked : Qt::Unchecked;
        }
        // Für Ordner: rekursiv prüfen welche Subelemente ausgewählt sind.
        else
        {
            float numChecked = 0;
            int   synced = 0;
            for (int i = 0; i < this->rowCount(); ++i)
            {
                switch (((Structureelement*) this->child(i))->data(role).toInt())
                {
                    case Qt::Checked:
                        numChecked++;
                        break;
                    case Qt::PartiallyChecked:
                        numChecked+=0.5;
                        break;
                    default:
                        if (((Structureelement*)this->child(i))->data(synchronisedRole) != NOT_SYNCHRONISED)
                            synced++;
                        break;
                }
            }
            if (numChecked == 0 || synced == this->rowCount()) return Qt::Unchecked;
            else if (numChecked + synced == this->rowCount() || typeEX == courseItem) return Qt::Checked;
            else return Qt::PartiallyChecked;
        }
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
        if (synchronised == JUST_SYNCHRONISED)
            return QBrush(Qt::blue);
        else if (synchronised == SYNCHRONISED
                 || typeEX != fileItem)
            return QBrush(Qt::black);
        else if (included)
            return QBrush(Qt::darkGreen);
        else
            return QBrush(Qt::red);
    }
    return QStandardItem::data(role);
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
        setCheckState(this->included ? Qt::Checked : Qt::Unchecked);
    }
    else if (role == Qt::CheckStateRole)
    {
        this->included = value.toInt() > 0 ? true : false;
        if (typeEX != fileItem && value != Qt::PartiallyChecked)
        {
            for (int i = 0; i < this->rowCount(); i++)
            {
                ((Structureelement*) this->child(i))->setData(value, role);
            }
        }
        // Ansicht aktualisieren - auch die Eltern-Elemente.
        emitDataChanged();
        if (parent())
        {
            parent()->setData(Qt::PartiallyChecked, role);
        }
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
