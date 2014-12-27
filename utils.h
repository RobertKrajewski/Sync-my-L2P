#ifndef UTILS_H
#define UTILS_H

#include <QObject>
#include <QClipboard>
#include <QMessageBox>
#include "structureelement.h"

class Utils : public QObject
{
    Q_OBJECT
public:
    explicit Utils(QObject *parent = 0);
    static QString getElementLocalPath(Structureelement* item, QString path);
    static void copyTextToClipboard(QString text);
    static void errorMessageBox(QString message, QString detailMessage);

    static QList<Structureelement*> getAllCourseItems(QStandardItemModel* itemModel);
    static Structureelement *getSemesterItem(QStandardItemModel *itemModel, QString semester);

    static Structureelement *getDirectoryItem(Structureelement *courseItem, QStringList path);
signals:
    
public slots:

};

#endif // UTILS_H
