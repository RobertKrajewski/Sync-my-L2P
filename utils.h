#ifndef UTILS_H
#define UTILS_H

#include <QObject>
#include "structureelement.h"

class Utils : public QObject
{
    Q_OBJECT
public:

    static QString getElementLocalPath(Structureelement* item, QString path, bool includeFilname = true, bool includePrefix = true);
    static void copyTextToClipboard(QString text);
    static void errorMessageBox(QString message, QString detailMessage);

    static QList<Structureelement*> getAllCourseItems(QStandardItemModel* itemModel);
    static Structureelement *getSemesterItem(QStandardItemModel *itemModel, QString semester);

    static Structureelement *getDirectoryItem(Structureelement *courseItem, QStringList path);

    static void centerWidgetOnDesktop(QWidget* widget);
private:
    explicit Utils(QObject *parent = 0);

};

#endif // UTILS_H
