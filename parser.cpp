#include "parser.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMap>
#include <QFile>

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
        QLOG_ERROR() << "Kursinformationen leer bzw. nicht lesbar.";
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
        Structureelement *newCourse = new Structureelement(title, cid, courseItem);

        Utils::getSemesterItem(itemModel, semester)->appendRow(newCourse);

        QLOG_INFO() << "Veranstaltung " << title << " (" << cid << ") hinzugefügt.";
    }
}

void Parser::parseFiles(QNetworkReply *reply, QMap<QNetworkReply*, Structureelement*> *replies, QString downloadDirectoryPath)
{
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
        QLOG_DEBUG() << "Status der Kursinformationen nicht ok.";
        return;
    }

    QJsonArray files = object["dataSet"].toArray();

    //QString baseUrl("https://www3.elearning.rwth-aachen.de");

    foreach(QJsonValue element, files)
    {
        QJsonObject file = element.toObject();
        QJsonObject fileInformation = file["fileInformation"].toObject();

        QString filename = fileInformation["fileName"].toString();
        int filesize = fileInformation["fileSize"].toString().toInt();
        int timestamp = fileInformation["modifiedTimestamp"].toInt();
        QString directory = file["sourceFolder"].toString();
        QString url = /*baseUrl %*/ fileInformation["downloadUrl"].toString();
        QStringList urlParts = url.split('/');

        urlParts.removeFirst();
        urlParts.removeFirst();
        urlParts.removeFirst();
        urlParts.removeFirst();
        urlParts.removeLast();

        Structureelement *dir = Utils::getDirectoryItem(currentCourse, urlParts);

        Structureelement* newFile = new Structureelement(filename, QUrl(url), timestamp, filesize);

        newFile->setData(currentCourse->data(cidRole), cidRole);

        newFile->setData(NOT_SYNCHRONISED, synchronisedRole);

        dir->appendRow(newFile);
    }
}
