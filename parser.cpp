#include "parser.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

#include "utils.h"
#include "qslog/QsLog.h"

void Parser::parseCourses(QNetworkReply *reply, QStandardItemModel *itemModel)
{
    // Empfangene Nachricht auslesen und als JSON interpretieren
    QByteArray response(reply->readAll());
    QJsonDocument document = QJsonDocument::fromJson(response);
    QJsonObject object = document.object();

    if(object.isEmpty())
    {
        QLOG_INFO() << tr("Kursinformationen leer bzw. nicht lesbar.");
        return;
    }

    if(!object["Status"].toBool())
    {
        QLOG_ERROR() << tr("Status der Kursinformationen nicht ok.");
        return;
    }

    // Array mit allen einzelnen Vorlesungen/Veranstaltungen
    QJsonArray courses = object["dataSet"].toArray();

    // Für jede Veranstaltung ein neues Strukturelement anlegen
    foreach(QJsonValue element, courses)
    {
        QJsonObject course = element.toObject();

        QString title = course["courseTitle"].toString();
        QString cid = course["uniqueid"].toString();
        QString semester = course["semester"].toString();
        QString url = course["url"].toString();

        // Erstellen eines RegExps  für unzulässige Buchstaben im Veranstaltungsnamen
        QString escapePattern = "(:|<|>|/|\\\\|\\||\\*|\\^|\\?|\\\")";
        QRegExp escapeRegExp(escapePattern, Qt::CaseSensitive);
        title.replace(escapeRegExp, "").trimmed();

        Structureelement *newCourse = new Structureelement(title, QUrl(url), 0, 0, cid, courseItem);

        Utils::getSemesterItem(itemModel, semester)->appendRow(newCourse);

        QLOG_INFO() << tr("Veranstaltung ") << title << " (" << cid << tr(") hinzugefügt.");
    }
}

void Parser::parseFiles(QNetworkReply *reply, QMap<QNetworkReply*, Structureelement*> *replies, QString downloadDirectoryPath)
{
    QString url = reply->url().toString();

    int responseCategory;

    // Kategorie der Daten bestimmen, um richtigen Parser auszuwählen
    if(url.contains("viewAllLearningMaterials"))
    {
        responseCategory = 0;
    }
    else if(url.contains("viewAllSharedDocuments"))
    {
        responseCategory = 1;
    }
    else if(url.contains("viewAllAssignments"))
    {
        responseCategory = 2;
    }
    else if(url.contains("viewAllMediaLibrarys"))
    {
        responseCategory = 3;
    }
    else if(url.contains("viewAllAnnouncements"))
    {
        responseCategory = 4;
    }
    else if(url.contains("viewAllEmails"))
    {
        responseCategory = 5;
    }
    else
    {
        QLOG_ERROR() << tr("Antwort auf unbekannten Request erhalten: ") << url;
        return;
    }

    Structureelement *currentCourse = replies->value(reply);

    QByteArray response = reply->readAll();

    QJsonDocument document = QJsonDocument::fromJson(response);
    QJsonObject object = document.object();

    if(object.isEmpty())
    {
        QLOG_DEBUG() << tr("Kursinformationen leer bzw. nicht lesbar.");
        return;
    }

    if(!object["Status"].toBool())
    {
        QLOG_ERROR() << tr("Status der Kursinformationen nicht ok.");
        return;
    }

    QJsonArray files = object["dataSet"].toArray();
    foreach(QJsonValue element, files)
    {
        QJsonObject file = element.toObject();
        QString filename;
        int filesize;
        int timestamp;
        QString url;
        QStringList urlParts;

        if(responseCategory == 0)
        {
            QJsonObject fileInformation = file["fileInformation"].toObject();

            filename = fileInformation["fileName"].toString();
            filesize = fileInformation["fileSize"].toString().toInt();
            timestamp = fileInformation["modifiedTimestamp"].toInt();
            url = fileInformation["downloadUrl"].toString();
            urlParts = url.split('/');

            urlParts.removeFirst();
            urlParts.removeFirst();
            urlParts.removeFirst();
            urlParts.removeFirst();
            urlParts.removeLast();
        }
        else if(responseCategory == 1)
        {
            // Wir brauchen nur die Dateien
            if(file["isDirectory"].toBool())
            {
                continue;
            }

            filename = file["name"].toString();
            filesize = -1;
            timestamp = file["modifiedTimestamp"].toInt();
            url = file["downloadUrl"].toString();
            urlParts = url.split('/');

            urlParts.removeFirst();
            urlParts.removeFirst();
            urlParts.removeFirst();
            urlParts.removeFirst();
            urlParts.removeFirst();
            urlParts.removeLast();
        }
        else if(responseCategory == 2)
        {
            QJsonArray assignmentDocs = file["assignmentDocuments"].toArray();

            foreach(QJsonValue assignmentElement, assignmentDocs)
            {
                QJsonObject assignmentDoc = assignmentElement.toObject();

                filename = assignmentDoc["fileName"].toString();
                filesize = assignmentDoc["fileSize"].toString().toInt();
                timestamp = assignmentDoc["modifiedTimestamp"].toInt();
                url = assignmentDoc["downloadUrl"].toString();
                urlParts = url.split('/');

                urlParts.removeFirst();
                urlParts.removeFirst();
                urlParts.removeFirst();
                urlParts.removeFirst();
                urlParts.removeFirst();
                urlParts.removeLast();

                Structureelement *dir = Utils::getDirectoryItem(currentCourse, urlParts);

                Structureelement* newFile = new Structureelement(filename, QUrl(url), timestamp, filesize,
                                                                 currentCourse->data(cidRole).toString(),
                                                                 fileItem);

                // Element hinzufügen
                dir->appendRow(newFile);
            }
        }
        else if(responseCategory == 3)
        {
            QJsonObject fileInformation = file["fileInformation"].toObject();

            filename = fileInformation["fileName"].toString();
            filesize = fileInformation["fileSize"].toString().toInt();
            timestamp = fileInformation["modifiedTimestamp"].toInt();
            url = fileInformation["downloadUrl"].toString();

            // Wir brauchen keine Vorschaubilder
            if(url.contains("Preview%20Images"))
                {
                    continue;
                }

            // Um eine lesbare Orderstruktur zu ermöglichen wird hier nicht die eigentliche URL, sondern folgender Konstrukt verwedet
            QString readurl = "|"+file["sourceFolder"].toString()+"/"+file["title"].toString();
            urlParts = readurl.split('/');
            urlParts.removeFirst();
            urlParts.removeFirst();
            urlParts.removeFirst();
            urlParts.removeFirst();
            urlParts.removeLast();

        }
        else if(responseCategory == 4 || responseCategory == 5)
        {
            if (!file["attachments"].isNull()) {
                QJsonArray attachment = file["attachments"].toArray();
                foreach(QJsonValue attachmentElement, attachment)
                {
                    QJsonObject fileInformation = attachmentElement.toObject();
                    filename = fileInformation["fileName"].toString();
                    filesize = fileInformation["fileSize"].toString().toInt();
                    timestamp = fileInformation["modifiedTimestamp"].toInt();
                    url = fileInformation["downloadUrl"].toString();
                    urlParts = url.split('/');
                    urlParts.removeFirst();
                    urlParts.removeFirst();
                    urlParts.removeFirst();
                    urlParts.removeFirst();
                    urlParts.removeLast();
                    urlParts.removeLast();

                    Structureelement *dir = Utils::getDirectoryItem(currentCourse, urlParts);

                    Structureelement* newFile = new Structureelement(filename, QUrl(url), timestamp, filesize,
                                                                     currentCourse->data(cidRole).toString(),
                                                                     fileItem);

                    // Element hinzufügen
                    dir->appendRow(newFile);

                }
            }
            else {
                continue;
            }
        }
        if(url.contains("Lehrproben"))
        {
            QLOG_INFO() << file["name"];
        }

        // Wegen der inneren foreach-Schleife in den Kategorien 2,4 und 5 ist diese Übergabe nur noch für die Kategorie 0, 1 und 3
        if(responseCategory == 0 ||responseCategory == 1 || responseCategory == 3){

            Structureelement *dir = Utils::getDirectoryItem(currentCourse, urlParts);

            Structureelement* newFile = new Structureelement(filename, QUrl(url), timestamp, filesize,
                                                         currentCourse->data(cidRole).toString(),
                                                         fileItem);

            // Element hinzufügen
            dir->appendRow(newFile);
        }
    }
}
