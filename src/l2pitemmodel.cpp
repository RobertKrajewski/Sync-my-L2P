#include "l2pitemmodel.h"
#include <QTextCodec>

#include <QStandardPaths>
#include <QCoreApplication>
#include <QThread>
#include "qslog/QsLog.h"
#include "urls.h"
#include "options.h"

L2pItemModel::L2pItemModel()
    : QObject()
{
    data = new QStandardItemModel();

    proxy.setDynamicSortFilter(true);
    proxy.setSourceModel(data);

    QObject::connect(&manager, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(serverDataRecievedSlot(QNetworkReply*)));
}

L2pItemModel::~L2pItemModel()
{
    if(data) delete data;
    if(oldData) delete oldData;
}

/**
 * @brief Laden der Daten vom L2P Server
 */
void L2pItemModel::loadDataFromServer()
{
    if(oldData)
    {
        oldData->deleteLater();
    }

    // Aktuelle Daten behalten für ein Merge mit den neuen Daten
    oldData = data;

    // Neues Datenmodell erstellen
    data = new QStandardItemModel();
    proxy.setSourceModel(data);

    numRequests = 0;

    // Request für Kurse starten
    requestCourses();
    requestMoodleCourses();
}

/**
 * @brief Senden eines Requests zum Erhalt aller ausgewählten Veranstaltungen
 */
void L2pItemModel::requestCourses()
{
    QLOG_DEBUG() << tr("Sende Request für Veranstaltungen");

    QString url = options->isCurrentSemesterCheckBoxChecked() ?
                  viewAllCourseInfoByCurrentSemesterUrl :
                  viewAllCourseInfoUrl;

    QUrl request_url(url % "?accessToken=" % options->getAccessToken());
    QNetworkRequest request(request_url);

    OpenRequest openRequest = {nullptr,
                               courses,
                               QTime::currentTime(),
                               request};
    requestQueue.append(openRequest);
    numRequests++;

    startNextRequests();
}

/**
 * @brief Senden eines Requests zum Erhalt aller ausgewählten Veranstaltungen von Moodle
 */
void L2pItemModel::requestMoodleCourses()
{
    QLOG_DEBUG() << tr("Sende Request für Veranstaltungen von Moodle");

    QString url = moodleGetMyEnrolledCoursesUrl;

    QString aktuelles_semester = "0";
    QString token = options->getAccessToken();

    QString tmp_url(url % "?token=" % token);
    // filter by current semester
    if (options->isCurrentSemesterCheckBoxChecked())
        tmp_url += "&semesterOffset=" % aktuelles_semester;
    QUrl request_url(tmp_url);
    QNetworkRequest request(request_url);


    OpenRequest openRequest = {nullptr,
                               moodleCourses,
                               QTime::currentTime(),
                               request};
    requestQueue.append(openRequest);
    numRequests++;

    startNextRequests();
}

/**
 * @brief Request für die aktiven Module aller Kurse
 */
void L2pItemModel::requestFeatures()
{
    QLOG_DEBUG() << tr("Sende Request für aktive Features");

    for(auto *course : Utils::getAllCourseItems(data))
    {
        auto system = course->data(systemEXRole);
        if (system == moodle) continue;

        QString request_url = viewActiveFeaturesUrl %
                                     "?accessToken=" % options->getAccessToken() %
                                     "&cid=" % course->data(cidRole).toString();
        QUrl url = request_url;
        QNetworkRequest request(url);

        OpenRequest openRequest = {course,
                                   features,
                                   QTime::currentTime(),
                                   request};
        requestQueue.append(openRequest);
        numRequests++;
    }
}

/**
 * @brief Request für die aktiven Module aller Kurse
 */
void L2pItemModel::requestMoodleFiles()
{

    for(auto *course : Utils::getAllCourseItems(data))
    {
        auto system = course->data(systemEXRole);
        if (system == l2p) continue;
        QString request_url = moodleGetFilesUrl %
                "?token=" % options->getAccessToken() %
                "&courseid=" % course->data(cidRole).toString();
        QUrl url = request_url;
        QNetworkRequest request(url);

        OpenRequest openRequest = {course,
                                   moodleFiles,
                                   QTime::currentTime(),
                                   request};
        requestQueue.append(openRequest);
        numRequests++;

        QLOG_DEBUG() << tr("Erstellter Moodle-Request:") << request_url;
    }
}

/**
 * @brief Auffinden und Einlesen des auf der Festplatte gespeicherten Dateibaums
 */
void L2pItemModel::loadDataFromFile()
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf-8"));

    // Dateipfad bestimmen
#if QT_VERSION >= 0x050400
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
#else
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#endif
    QLOG_DEBUG() << tr("Vermuteter Pfad der Progammdaten: ") << dataPath;

    // Öffnen der Datei
    QFile dataFile(dataPath + "/" + DATAFILENAME );
    if(!dataFile.open(QIODevice::ReadOnly))
    {
        QLOG_ERROR() << tr("Kann keine Daten von Festplatte laden") << ": " << dataFile.errorString();
        return;
    }

    // Dateiinhalt überprüfen
    QTextStream ts(&dataFile);
    ts.setCodec(QTextCodec::codecForName("utf-8"));
    if(ts.atEnd())
    {
        QLOG_INFO() << tr("Geladene Datei enthält keine Daten.");
        return;
    }

    // Dateiinhalt einlesen
    QDomDocument dom;
    QString errorMessage;
    if(!dom.setContent(ts.readAll(), &errorMessage))
    {
        QLOG_ERROR() << tr("Kann Daten von Festplatte nicht fehlerfrei einlesen: ") << errorMessage;
        return;
    }

    // Schließen der Datei
    dataFile.close();

    // Parsen der gelesenen XML-Daten
    QDomElement root = dom.documentElement();
    parseDataFromXml(root, data->invisibleRootItem());

    // Überprüfung auf Vorhandensein auf der Festplatte
    QList<Structureelement*> items;
    getItemList(data->invisibleRootItem(), items);
    Utils::checkAllFilesIfSynchronised(items, options->downloadFolderLineEditText());
}

void L2pItemModel::saveDataToFile()
{
    QDomDocument domDoc;
    parseDataToXml(domDoc, data->invisibleRootItem(), nullptr);

#if QT_VERSION >= 0x050400
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
#else
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#endif

    if(!QDir(dataPath).exists() && !QDir().mkpath(dataPath))
    {
        Utils::errorMessageBox(tr("Pfad nicht erstellbar"),
                               tr("Konnte Pfad für Speicherung der Kursinformationen nicht erstellen") + " (" + dataPath + ")");
    }

    QFile file(dataPath + "/" + DATAFILENAME);
    if(!file.open(QIODevice::WriteOnly))
    {
        Utils::errorMessageBox(tr("Kursinformationen nicht speicherbar."),
                               file.errorString());
        return;
    }
    QTextStream ts(&file);
    ts.setCodec(QTextCodec::codecForName("utf-8"));
    ts << domDoc.toString();
    file.close();
}

/**
 * @brief Rekursive Umwandlung eines XML Baums in das Datenmodell
 * @param input Eingabe Node des XML Baums
 * @param parentItem Item, dem eingelesene Daten angefügt werden
 */
void L2pItemModel::parseDataFromXml(QDomElement input, QStandardItem *parentItem)
{
    // Abbrechen bei ungültigen Eingabedaten
    if(input.isNull() || parentItem == nullptr)
    {
        return;
    }

    // Daten des aktuellen Knoten laden
    if(input.tagName() == "item")
    {
        QString name = input.attribute("name", "");
        QUrl url = QUrl(input.attribute("url", ""));
        int time = input.attribute("time", "0").toInt();
        int size = input.attribute("size", "0").toInt();
        QString cid = input.attribute("cid", "");
        MyItemType type = static_cast<MyItemType>(input.attribute("type", "").toInt());
        MyItemSystem system = static_cast<MyItemSystem>(input.attribute("system", "").toInt());
        bool included = input.attribute("included", "0").toInt();

        // Neues
        QStandardItem *newItem = new Structureelement(name, url, time, size, cid, type, system);
        newItem->setData(included, includeRole);

        parentItem->appendRow(newItem);
        parentItem = newItem;
    }

    // Alle Kindknoten hinzufügen
    QDomNodeList children = input.childNodes();
    for(int i=0; i < children.length(); i++)
    {
        parseDataFromXml(children.item(i).toElement(), parentItem);
    }
}

/**
 * @brief Rekursive Umwandlung des Datenbaums in einen XML Baum
 * @param output Ausgabenode des XML Baums
 * @param item Auszugebendes Item
 * @param parentItem Eltern XML Element, dem das neue Item hinzugefügt wird
 */
void L2pItemModel::parseDataToXml(QDomDocument &output, QStandardItem *item,
                                  QDomElement *parentItem)
{
    QDomElement xmlItem;

    // Das Root-item auslassen
    if(parentItem == nullptr)
    {
        xmlItem = output.createElement("root");
        output.appendChild(xmlItem);
    }
    else
    {
        xmlItem = output.createElement("item");
        xmlItem.setAttribute("name", item->text());
        xmlItem.setAttribute("url", item->data(urlRole).toUrl().toString());
        xmlItem.setAttribute("time", item->data(dateRole).toDateTime().toMSecsSinceEpoch()/1000);
        xmlItem.setAttribute("size", item->data(sizeRole).toInt());
        xmlItem.setAttribute("cid", item->data(cidRole).toString());
        xmlItem.setAttribute("type", item->type());
        xmlItem.setAttribute("system", item->data(systemEXRole).toInt());
        xmlItem.setAttribute("included", item->data(includeRole).toBool());
        parentItem->appendChild(xmlItem);
    }

    for(int i=0; i < item->rowCount(); i++)
    {
        parseDataToXml(output, item->child(i), &xmlItem);
    }
}

/**
 * @brief Hinzufügen aller Kurse aus der Antwort des Servers
 * @param reply Netzwerkantwort mit Kursen
 */
void L2pItemModel::addCoursesFromReply(QNetworkReply *reply)
{
    if(reply->error())
    {
        QLOG_ERROR() << tr("Beim Abruf der Veranstaltungen ist ein Fehler aufgetreten") % reply->errorString() % ";\n " % reply->url().toString();
    }
    else
    {
        QLOG_INFO() << tr("Veranstaltungen empfangen");
        Parser::parseCourses(reply, data);
    }

    if(data->rowCount() != 0)
    {
        // Veranstaltungen alphabetisch sortieren
        data->sort(0);

        // Aktive Features abrufen
        requestFeatures();
    }
}

/**
 * @brief Hinzufügen aller Moodle-Kurse aus der Antwort des Servers
 * @param reply Netzwerkantwort mit Moodle-Kursen
 */
void L2pItemModel::addMoodleCoursesFromReply(QNetworkReply *reply)
{
    if(reply->error())
    {
        QLOG_ERROR() << tr("Beim Abruf der Moodle-Veranstaltungen ist ein Fehler aufgetreten") % reply->errorString() % ";\n " % reply->url().toString();
    }
    else
    {
        QLOG_INFO() << tr("Moodle-Veranstaltungen empfangen");
        // data ist eine liste mit semestern, diese semester enthalten course
        // reply enthält eine liste mit courses
        // wird dann in json gecastet
        // dann für alle course in der liste ein course (structureelement) daraus erstellt
        // dann werden diese course zu data ins jeweilige semester hinzugefügt
        Parser::parseMoodleCourses(reply, data);
    }

    if(data->rowCount() != 0)
    {
        // Veranstaltungen alphabetisch sortieren
        data->sort(0);

        // Moodle Files abrufen
        requestMoodleFiles();
    }
}

void L2pItemModel::addFeatureFromReply(QNetworkReply *reply, Structureelement *course)
{
    const auto activeFeatures = Parser::parseFeatures(reply);

    QNetworkAccessManager &manager = *(new QNetworkAccessManager());
    QObject::connect(&manager, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(serverDataRecievedSlot(QNetworkReply*)));

    if(options->isLearningMaterialsCheckBoxChecked() && activeFeatures.contains("Learning Materials"))
    {
        OpenRequest request = {course,
                               files,
                               QTime::currentTime(),
                               createApiRequest(course, "viewAllLearningMaterials")};
        requestQueue.append(request);
        numRequests++;
    }

    if(options->isSharedLearningmaterialsCheckBoxChecked() && activeFeatures.contains("Shared Documents"))
    {
        OpenRequest request = {course,
                               files,
                               QTime::currentTime(),
                               createApiRequest(course, "viewAllSharedDocuments")};
        requestQueue.append(request);
        numRequests++;
    }

    if(options->isAssignmentsCheckBoxChecked() && activeFeatures.contains("Assignments"))
    {
        OpenRequest request = {course,
                               files,
                               QTime::currentTime(),
                               createApiRequest(course, "viewAllAssignments")};
        requestQueue.append(request);
        numRequests++;
    }

    if(options->isMediaLibrarysCheckBoxChecked() && activeFeatures.contains("Media Library"))
    {
        OpenRequest request = {course,
                               files,
                               QTime::currentTime(),
                               createApiRequest(course, "viewAllMediaLibraries")};
        requestQueue.append(request);
        numRequests++;
    }

    if(options->isAnnouncementAttachmentsCheckBoxChecked() && activeFeatures.contains("Announcements"))
    {
        OpenRequest request = {course,
                               files,
                               QTime::currentTime(),
                               createApiRequest(course, "viewAllAnnouncements")};
        requestQueue.append(request);
        numRequests++;
    }

    if(options->isEmailAttachmentsCheckBoxChecked() && activeFeatures.contains("Emails"))
    {
        OpenRequest request = {course,
                               files,
                               QTime::currentTime(),
                               createApiRequest(course, "viewAllEmails")};
        requestQueue.append(request);
        numRequests++;
    }

    if(options->isTutorDomainCheckBoxChecked() && activeFeatures.contains("TutorDomain"))
    {
        OpenRequest request = {course,
                               files,
                               QTime::currentTime(),
                               createApiRequest(course, "viewAllTutorDomainDocuments")};
        requestQueue.append(request);
        numRequests++;
    }

    QLOG_DEBUG() << "Current open requests: " << replies.size();
}

void L2pItemModel::addFilesFromReply(QNetworkReply *reply, Structureelement *course)
{
    QLOG_DEBUG() << tr("Dateiinformationen empfangen: ") << reply->url().toString();

    // Prüfen auf Fehler
    if (!reply->error())
    {
        Parser::parseFiles(reply, course);

        QLOG_DEBUG() << tr("Dateiinformationen geparst: ") << reply->url().toString();
    }
    else
    {
        QString replyMessage(reply->readAll());

        if(replyMessage.contains("secure channel"))
        {
            QLOG_DEBUG() << tr("SSL Fehler für: ") << reply->url().toString();
        }
        else
        {
            auto errorMessage = reply->errorString();
            QLOG_ERROR() << tr("Beim Abruf der Veranstaltungen ist ein Fehler aufgetreten") % reply->errorString() % ";\n " % reply->url().toString() % replyMessage;
        }
    }

    // Prüfen, ob alle Antworten bearbeitet wurden
    if (replies.empty())
    {

        QList<Structureelement*> items;

        QStandardItem* root = data->invisibleRootItem();
        for( int i=0; i < root->rowCount(); i++)
        {
            getItemList(static_cast<Structureelement*>(root->child(i)), items);
        }

        if(oldData)
        {
            QList<Structureelement*> oldItems;

            // Get old data
            root = oldData->invisibleRootItem();
            getItemList(root, oldItems);

            foreach(Structureelement *item, items)
            {
                // Find an old item which fits to a new one and copy properties
                for(auto it = oldItems.begin(); it != oldItems.end(); it++)
                {
                    auto *oldItem = *it;
                    if(item->data(urlRole) == oldItem->data(urlRole) && item->text() == oldItem->text())
                    {
                        item->setData(oldItem->data(includeRole), includeRole);
                        oldItems.erase(it);
                        break;
                    }
                }

                // Don't include item if parent is not included
                auto* parentItem = dynamic_cast<Structureelement*>(item->parent());
                if(parentItem && !parentItem->data(includeRole).toBool() && item->data(includeRole).toBool())
                {
                    item->setData(false, includeRole);
                }
            }
        }

        Utils::checkAllFilesIfSynchronised(items, options->downloadFolderLineEditText());
    }
}

void L2pItemModel::addMoodleFilesFromReply(QNetworkReply *reply, Structureelement *course)
{
    QLOG_DEBUG() << tr("Moodle-Dateiinformationen empfangen: ") << reply->url().toString();

    // Prüfen auf Fehler
    if (!reply->error())
    {
        Parser::parseMoodleFiles(reply, course);

        QLOG_DEBUG() << tr("Moodle-Dateiinformationen geparst: ") << reply->url().toString();
    }
    else
    {
        QString replyMessage(reply->readAll());

        if(replyMessage.contains("secure channel"))
        {
            QLOG_DEBUG() << tr("SSL Fehler für: ") << reply->url().toString();
        }
        else
        {
            auto errorMessage = reply->errorString();
            QLOG_ERROR() << tr("Beim Abruf der Veranstaltungen ist ein Fehler aufgetreten") % reply->errorString() % ";\n " % reply->url().toString() % replyMessage;
        }
    }

    // Prüfen, ob alle Antworten bearbeitet wurden
    if (replies.empty())
    {

        QList<Structureelement*> items;

        QStandardItem* root = data->invisibleRootItem();
        for( int i=0; i < root->rowCount(); i++)
        {
            getItemList(static_cast<Structureelement*>(root->child(i)), items);
        }

        if(oldData)
        {
            QList<Structureelement*> oldItems;

            // Get old data
            root = oldData->invisibleRootItem();
            getItemList(root, oldItems);

            foreach(Structureelement *item, items)
            {
                // Find an old item which fits to a new one and copy properties
                for(auto it = oldItems.begin(); it != oldItems.end(); it++)
                {
                    auto *oldItem = *it;
                    if(item->data(urlRole) == oldItem->data(urlRole) && item->text() == oldItem->text())
                    {
                        item->setData(oldItem->data(includeRole), includeRole);
                        oldItems.erase(it);
                        break;
                    }
                }

                // Don't include item if parent is not included
                auto* parentItem = dynamic_cast<Structureelement*>(item->parent());
                if(parentItem && !parentItem->data(includeRole).toBool() && item->data(includeRole).toBool())
                {
                    item->setData(false, includeRole);
                }
            }
        }

        Utils::checkAllFilesIfSynchronised(items, options->downloadFolderLineEditText());
    }
}

/**
 * @brief Erstellung eines Lernraumspezifischen Requests
 * @param course Strukturelement des Kurses
 * @param apiCommand auszuführender Befehl
 * @return
 */
QNetworkRequest L2pItemModel::createApiRequest(Structureelement *course,
                                               QString apiCommand)
{
    QString access = "?accessToken=" % options->getAccessToken();
    QString cid = "&cid=" % course->data(cidRole).toString();

    QString url = l2pApiUrl % apiCommand % access % cid;
    QNetworkRequest request(QUrl(QUrl::toPercentEncoding(url, ":/?=&")));

    QLOG_DEBUG() << tr("Erstellter Request:") << url;

    return request;
}

/**
 * @brief Verarbeitung empfangener Nachrichten vom L2P Server
 * @param reply empfangene Nachricht
 */
void L2pItemModel::serverDataRecievedSlot(QNetworkReply *reply)
{
    if(!replies.contains(reply))
    {
        QLOG_ERROR() << tr("Unerwartete Serverantwort erhalten") % ":" % reply->url().toString();
        return;
    }

    // Antwortinformationen auslesen
    ReplyInfo replyInfo = replies.value(reply);
    replies.remove(reply);

    QTime elapsed = QTime::fromMSecsSinceStartOfDay(QTime::currentTime().msecsSinceStartOfDay() - replyInfo.timeStart.msecsSinceStartOfDay());
    QLOG_DEBUG() << "Elapsed time: " << elapsed.toString() << " for request url: " << reply->url().toString();
    emit showStatusMessage(QString("Aktualisierungsfortschritt: %1 von %2 Anfragen durchgeführt").arg(numRequests-requestQueue.size()).arg(numRequests));

    switch (replyInfo.type)
    {
    case courses:
    {
        addCoursesFromReply(reply);
        break;
    }
    case moodleCourses:
    {
        addMoodleCoursesFromReply(reply);
        break;
    }
    case features:
    {
        addFeatureFromReply(reply, replyInfo.item);
        break;
    }
    case files:
    {
        addFilesFromReply(reply, replyInfo.item);
        break;
    }
    case moodleFiles:
        addMoodleFilesFromReply(reply, replyInfo.item);
        break;
    default:
    {
        QLOG_ERROR() << tr("Serverantwort wurde unbekannter Typ zugeordnet") % ":" % reply->url().toString();
    }
    }

    startNextRequests();

    reply->deleteLater();

    // Keine weiteren Antworten zu bearbeiten
    if(replies.empty())
    {
        // Alle Dateien nach Namen sortieren
        data->sort(0);

        QLOG_DEBUG() << tr("Aktualisierung beendet");
        emit loadingFinished(true);
    }
}

void L2pItemModel::startNextRequests()
{
    while(replies.size() < 19 && requestQueue.size() > 0)
    {
        auto nextRequest = requestQueue.first();
        ReplyInfo replyInfo = {nextRequest.item, nextRequest.type, QTime::currentTime()};
        replies.insert(manager.get(nextRequest.request),
                       replyInfo);
        requestQueue.pop_front();
    }
}

/**
 * @brief L2pItemModel::getItemList
 * @param topElement
 * @param list
 */
void L2pItemModel::getItemList(QStandardItem *topElement, QList<Structureelement *> &list)
{
    Structureelement *item = dynamic_cast<Structureelement*>(topElement);
    if(item)
    {
        list.append(item);
    }

    // Alle Kindelemente hinzufügen
    for (int i = 0; i < topElement->rowCount(); i++)
        getItemList(topElement->child(i), list);
}
