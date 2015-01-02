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
        QLOG_INFO() << "Kursinformationen leer bzw. nicht lesbar.";
        return;
    }

    if(!object["Status"].toBool())
    {
        QLOG_ERROR() << "Status der Kursinformationen nicht ok.";
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

        QLOG_INFO() << "Veranstaltung " << title << " (" << cid << ") hinzugefügt.";
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
        return;
    }
    else if(url.contains("viewAllMediaLibrarys"))
    {
        responseCategory = 3;
        return;
    }
    else
    {
        QLOG_ERROR() << "Antwort auf unbekannten Request erhalten: " << url;
        return;
    }

    Structureelement *currentCourse = replies->value(reply);

    QByteArray response = reply->readAll();

    QJsonDocument document = QJsonDocument::fromJson(response);
    QJsonObject object = document.object();

    if(object.isEmpty())
    {
        QLOG_DEBUG() << "Kursinformationen leer bzw. nicht lesbar.";
        return;
    }

    if(!object["Status"].toBool())
    {
        QLOG_ERROR() << "Status der Kursinformationen nicht ok.";
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

        if(responseCategory == 0 || responseCategory == 3)
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
                filesize = assignmentDoc["fileSize"].toInt();
                timestamp = assignmentDoc["modifiedTimestamp"].toInt();
                url = file["downloadUrl"].toString();
                urlParts = url.split('/');

                urlParts.removeFirst();
                urlParts.removeFirst();
                urlParts.removeFirst();
                urlParts.removeFirst();
                urlParts.removeFirst();
                urlParts.removeLast();
            }
        }


        Structureelement *dir = Utils::getDirectoryItem(currentCourse, urlParts);

        Structureelement* newFile = new Structureelement(filename, QUrl(url), timestamp, filesize,
                                                         currentCourse->data(cidRole).toString(),
                                                         fileItem);

        // Element hinzufügen
        dir->appendRow(newFile);
    }
}
