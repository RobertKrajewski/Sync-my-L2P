#ifndef DATEI_H
#define DATEI_H
#include <strukturelement.h>


class Datei : public Strukturelement
{
public:
    Datei(QString name, QUrl url, QDateTime zeit, qint32 size);
    Datei(QString name, QUrl url, QString zeit, qint32 size);
    QDateTime GetZeit() const;
    qint32 getSize() const;

private:
    QDateTime   zeit;
};

#endif // DATEI_H
