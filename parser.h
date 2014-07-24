#ifndef PARSER_H
#define PARSER_H

#include <QObject>
#include <QNetworkReply>
#include <QStandardItemModel>
#include "structureelement.h"
#include <QXmlStreamReader>
#include <QMap>
#include <QFile>
#include "mymainwindow.h"

class Parser : public QObject
{
    Q_OBJECT
public:
    explicit Parser(QObject *parent = 0);
    static void parseCourses(QNetworkReply *reply, QStandardItemModel *itemModel);
    static void parseFiles(QNetworkReply *reply, QMap<QNetworkReply*, Structureelement*> *replies, QString downloadDirectoryPath);
    
signals:
    
public slots:
    
};

#endif // PARSER_H
