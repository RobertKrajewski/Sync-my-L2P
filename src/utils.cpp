#include <QStringBuilder>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QDesktopWidget>
#include <QFileInfo>
#include <QLinkedList>

#include "utils.h"
#include "urls.h"

#include "qslog/QsLog.h"

Utils::Utils(QObject *parent) :
    QObject(parent)
{
}

/// Bestimmung des lokalen Pfads für ein Element
Structureelement *Utils::getParentCourse(Structureelement *item)
{
    Structureelement* currentItem = item;
    while(currentItem)
    {
        if(currentItem->type() == courseItem)
        {
            return currentItem;
        }

        currentItem = dynamic_cast<Structureelement*>(currentItem->parent());
    }

    return nullptr;
}

QString Utils::getElementLocalPath(Structureelement *item, QString downloadDirectoryPath, bool includeFilname, bool includePrefix)
{
        QString path;

        // Zwischenverzeichnisse
        Structureelement* parent = item;
        while ((parent = (Structureelement*) parent->parent()) != 0)
        {
            auto element_text = parent->text();
            if(element_text.length() > 75)
                element_text = element_text.left(75).trimmed();
            path.push_front(element_text % "/");
        }

        // Downloadverzeichnis
        path.push_front(downloadDirectoryPath % "/");

        // Fileprefix hinzufügen
        if(includePrefix)
        {
            path.push_front("file:///");
        }

        // Dateiname
        if(includeFilname)
        {
            path.append(item->text());
        }

        return path;
}

QString Utils::getElementRemotePath(Structureelement *item)
{

    QString remoteUrl;
    auto typeEX = item->data(typeEXRole);
    auto systemEX = item->data(systemEXRole);

    if(typeEX == courseItem)
    {
        remoteUrl = item->data(urlRole).toString();
    }
    else if (typeEX == directoryItem)
    {
        return "";
    }
    else if (systemEX == l2p)
    {
        remoteUrl = item->data(urlRole).toString();
        // Ersten drei Zeichen entfernen, da der URL ein "|" vorangestellt ist + ein Zeichen /
        remoteUrl.remove(0,4);
        remoteUrl.prepend(l2pApiUrl);
    }
    else
    {
        // shows the file over the api.
        QString downloadurl = item->data(urlRole).toUrl().toDisplayString(QUrl::FullyDecoded);
        remoteUrl = moodleMainUrl + downloadurl;
    }
    return remoteUrl;
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

    QLOG_ERROR() << message << ": " << detailMessage;
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
            QLOG_ERROR() << tr("Unbekanntes Element auf der Ebene der Veranstaltungen: ") << element->text();
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
        // Remove unnessescary whit
        item = item.simplified();

        // Bei Verwendung der deutschen Sprache die Ordner umbennen
        if(QLocale::system().language() == QLocale::German)
        {
            if (item.contains("SharedDocuments")) {
                item = "Gemeinsame-Dokumente";
            }
            else if (item.contains("StructuredMaterials")) {
                item = "Lernmaterialien";
            }
            else if (item.contains("LA_AssignmentDocuments")) {
                item = "Übungsdokumente";
            }
            else if (item.contains("LA_SolutionDocuments")) {
                item = "Übungslösungen";
            }
            else if (item.contains("LA_CorrectionDocuments")) {
                item = "Übungskorrektur";
            }
            else if (item.contains("LA_SampleSolutions")) {
                item = "Übungsmusterlösung";
            }
            else if (item.contains("EmailAttachments")) {
                item = "E-Mails";
            }
            else if (item.contains("MediaLibrary")) {
                item = "Medienbibliothek";
            }
            else if (item.contains("AnnouncementDocuments")) {
                item = "Ankündigungen";
            }
            else if (item.contains("Announcement"))
            {
                item = "Ankündigungen";
            }
        }

        // Bei anderen Sprachen werden Anhänge zu den Nachrichten gepackt.
        if(QLocale::system().language() != QLocale::German)
        {
            if (item.contains("AnnouncementDocuments")) {
                item = "Announcement";
            }
            else if (item.contains("EmailAttachments")) {
                item = "E-Mails";
            }
        }

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

/// Zentrieren eines Fenster auf dem Desktops
void Utils::centerWidgetOnDesktop(QWidget *widget)
{
    QRect desktopRect = QApplication::desktop()->screenGeometry();
    QRect windowRect  = widget->frameGeometry();
    widget->move((desktopRect.width()-windowRect.width())/2+desktopRect.x(), (desktopRect.height()-windowRect.height())/2+desktopRect.y());
}

/// Überprüfung aller Dateien, ob diese auf der Festplatte bereits existieren
void Utils::checkAllFilesIfSynchronised(QList<Structureelement*> items, QString downloadDirectory)
{
    foreach(Structureelement* item, items)
    {
        if(item->type() != fileItem)
        {
            continue;
        }

        QString filePath = getElementLocalPath(item, downloadDirectory, true, false);
        QFileInfo fileInfo(filePath);

        if(fileInfo.exists() && fileInfo.isFile() &&
                fileInfo.size() == item->data(sizeRole).toInt()/* &&
                fileInfo.lastModified() == item->data(dateRole).toDateTime()*/)
            {
                item->setData(SYNCHRONISED, synchronisedRole);
            }
            else
            {
                item->setData(NOT_SYNCHRONISED, synchronisedRole);
            }

    }
}
