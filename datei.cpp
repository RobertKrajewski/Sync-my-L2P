#include "datei.h"

Datei::Datei(QString name, QUrl url, QDateTime zeit, qint32 groesse)
    :Strukturelement(name, url, fileItem), zeit(zeit)
{
    this->size = groesse;
}

Datei::Datei(QString name, QUrl url, QString zeit, qint32 groesse)
    : Strukturelement(name, url, fileItem), zeit(QDateTime::fromString(zeit, Qt::ISODate))
{
    this->size = groesse;
}

QDateTime Datei::GetZeit() const
{
    return zeit;
}


qint32 Datei::getSize() const
{
    return size;
}
