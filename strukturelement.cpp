#include "strukturelement.h"
#include "datei.h"

Strukturelement::Strukturelement(QString name, QUrl url, MyItemType type)
    :QStandardItem(name), einschliessen(true), url(url), typeEX(type)
{
    size = 0;
}

qint32 Strukturelement::getSize() const
{
    return size;
}

QVariant Strukturelement::data(int role) const
{
    if (role == includeRole)
    {
        return einschliessen;
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
    else if (role == Qt::StatusTipRole)
    {
        QString tooltip;

        if (typeEX == fileItem)
        {
            tooltip.append(text() % " - ");
            if (size > 1048576)
                tooltip.append(QString::number(size/1048576.0,'f',2) % " MB");
            else if(size > 1024)
                 tooltip.append(QString::number(size/1024.0,'f',2) % " KB");
            else
                 tooltip.append(QString::number(size) % " Byte");

            tooltip.append(" - " % ((Datei*)this)->GetZeit().toString("ddd dd.MM.yy hh:mm"));

            return tooltip;
        }
        return tooltip;
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
        if (einschliessen)
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
        this->einschliessen = value.toBool();
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
    else
    {
        QStandardItem::setData(value, role);
    }
}
