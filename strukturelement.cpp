#include "strukturelement.h"
#include "datei.h"

Strukturelement::Strukturelement(QString name, QUrl url, MyItemType type)
    :QStandardItem(name), included(true), url(url), typeEX(type)
{
    synchronised = false;
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
        return zeit;
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

            statustip.append(" - " % ((Datei*)this)->GetZeit().toString("ddd dd.MM.yy hh:mm"));

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
            if (synchronised)
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
        this->zeit = value.toDateTime();
    }
    else if (role == synchronisedRole)
    {
        this->synchronised = value.toBool();
    }
    else
    {
        QStandardItem::setData(value, role);
    }
}
