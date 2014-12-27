#include <QStringBuilder>
#include <QApplication>

#include "utils.h"

#include "qslog/QsLog.h"

Utils::Utils(QObject *parent) :
    QObject(parent)
{
}

/// Bestimmung des lokalen Pfads für ein Element
QString Utils::getElementLocalPath(Structureelement *item, QString downloadDirectoryPath)
{
        QString elementPath;

        // Dateiname
        elementPath.append(item->text());

        // Zwischenverzeichnisse
        Structureelement* parent = item;
        while ((parent = (Structureelement*) parent->parent()) != 0)
            elementPath.push_front(parent->text() % "/");

        // Downloadverzeichnis
        elementPath.push_front("file:///" % downloadDirectoryPath % "/");

        return elementPath;
}

void Utils::copyTextToClipboard(QString text)
{
    // Holen der globalen Zwischenablage
    QClipboard *clipboard = QApplication::clipboard();

    // Kopieren der URL des mit der rechten Maustaste geklickten Items in die Zwischenablage
    clipboard->setText(text);
}

void Utils::errorMessageBox(QString message, QString detailMessage)
{
    // Falls ein Fehler aufgetreten sein sollte, Ausgabe dessen
    QMessageBox messageBox;
    messageBox.setWindowIcon(QIcon(":/icons/magnifier.png"));
    messageBox.setText(message);
    messageBox.setInformativeText(detailMessage);
    messageBox.setStandardButtons(QMessageBox::Ok);
    messageBox.exec();
}

/// Erstellung einer Liste mit allen Veransaltungen
QList<Structureelement*> Utils::getAllCourseItems(QStandardItemModel *itemModel)
{
    QList<Structureelement*> courses;

    for(int row=0; row < itemModel->rowCount(); ++row)
    {
        Structureelement *element = static_cast<Structureelement*>(itemModel->item(row));
        if(element->type() == courseItem)
        {
            courses.append(element);
        }
        else if(element->type() == semesterItem)
        {
            for(int innerRow=0; innerRow < element->rowCount(); ++innerRow)
            {
                courses.append(static_cast<Structureelement*>(element->child(innerRow)));
            }
        }
        else
        {
            QLOG_ERROR() << "Unbekanntes Element auf der Ebene der Veranstaltungen: " << element->text();
        }
    }

    return courses;
}

/// Factory für SemesterItems
Structureelement *Utils::getSemesterItem(QStandardItemModel *itemModel, QString semester)
{
    QList<QStandardItem*> foundItems = itemModel->findItems(semester);
    Structureelement *semesterElement;

    if(foundItems.size())
    {
        semesterElement = static_cast<Structureelement*>(foundItems.first());
    }
    else
    {
        semesterElement = new Structureelement(semester, QUrl(), 0, 0, QString(), semesterItem);
        itemModel->appendRow(semesterElement);
    }

    return semesterElement;
}

/// Factory für DirectoryItems
Structureelement *Utils::getDirectoryItem(Structureelement *courseItem, QStringList path)
{
    Structureelement *currentItem = courseItem;

    // Iteriere entlang der Elemente des Pfads und erstelle diese ggf.
    foreach(QString item, path)
    {
        bool correctChildFound = false;
        for(int row=0; row < currentItem->rowCount(); ++row)
        {
            Structureelement *child = static_cast<Structureelement*>(currentItem->child(row));
            if(child->text() == item)
            {
                currentItem = child;
                correctChildFound = true;
                break;
            }
        }

        if(!correctChildFound)
        {
            Structureelement* child = new Structureelement(item, QUrl(), 0, 0, QString(), directoryItem);

            currentItem->appendRow(child);
            currentItem = child;
        }
    }

    return currentItem;
}
