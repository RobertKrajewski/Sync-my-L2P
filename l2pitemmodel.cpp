#include "l2pitemmodel.h"
#include <QTextCodec>

#include <QStandardPaths>

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
 * @brief Senden eines Requests zum Erhalt aller ausgewählten Veranstaltungen
 */
void L2pItemModel::requestCourses()
{
    QLOG_DEBUG() << tr("Sende Request für Veranstaltungen");

    QString url = options->isCurrentSemesterCheckBoxChecked() ?
                viewAllCourseInfoByCurrentSemesterUrl :
                viewAllCourseInfoUrl;

    QNetworkRequest request(QUrl(url % "?accessToken=" % options->getAccessToken()));
    replies.insert(manager.get(request), {nullptr, ReplyInfo::courses});
}

/**
 * @brief Request für die aktiven Module aller Kurse
 */
void L2pItemModel::requestFeatures()
{
    QLOG_DEBUG() << tr("Sende Request für aktive Features");

    for(auto *course : Utils::getAllCourseItems(data))
    {
        QNetworkRequest request(QUrl(viewActiveFeaturesUrl %
                                     "?accessToken=" % options->getAccessToken() %
                                     "&cid=" % course->data(cidRole).toString()));

        replies.insert(manager.get(request), {course, ReplyInfo::features});
    }
}

/**
 * @brief Laden der Daten vom L2P Server
 */
void L2pItemModel::loadDataFromServer()
{
    // Aktuelle Daten behalten für ein Merge mit den neuen Daten
    if(oldData)
    {
        oldData->deleteLater();
    }

    oldData = data;

    // Neues Datenmodell erstellen
    data = new QStandardItemModel();
    proxy.setSourceModel(data);

    // Request für Kurse starten
    requestCourses();
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
    parseDataToXml(domDoc, data->invisibleRootItem(), NULL);

#if QT_VERSION >= 0x050400
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
#else
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#endif

    if(!QDir(dataPath).exists())
    {
        QDir().mkdir(dataPath);
    }

    QFile file(dataPath + "/" + DATAFILENAME);
    if(!file.open(QIODevice::WriteOnly))
    {
        Utils::errorMessageBox(tr("Kursinformationen nicht speicherbar."),
                               file.errorString());
        return;
    }
    QTextStream ts(&file);
    ts << domDoc.toByteArray();
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
        bool included = input.attribute("included", "0").toInt();

        // Neues
        QStandardItem *newItem = new Structureelement(name, url, time, size, cid, type);
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
    if(item->text().isEmpty())
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
        Utils::errorMessageBox(tr("Beim Abruf der Veranstaltungen ist ein Fehler aufgetreten"),
                               reply->errorString() % ";\n " % reply->readAll());
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
    else
    {
        emit loadingFinished(true);
    }
}

void L2pItemModel::addFeatureFromReply(QNetworkReply *reply, Structureelement *course)
{
    const auto activeFeatures = Parser::parseFeatures(reply);

    if(options->isLearningMaterialsCheckBoxChecked() && activeFeatures.contains("Learning Materials"))
    {
        replies.insert(manager.get(createApiRequest(course, "viewAllLearningMaterials")),
                       {course, ReplyInfo::files});
    }

    if(options->isSharedLearningmaterialsCheckBoxChecked() && activeFeatures.contains("Shared Documents"))
    {
        replies.insert(manager.get(createApiRequest(course, "viewAllSharedDocuments")),
                       {course, ReplyInfo::files});
    }

    if(options->isAssignmentsCheckBoxChecked() && activeFeatures.contains("Assignments"))
    {
        replies.insert(manager.get(createApiRequest(course, "viewAllAssignments")),
        {course, ReplyInfo::files});
    }

    if(options->isMediaLibrarysCheckBoxChecked() && activeFeatures.contains("Media Library"))
    {
        replies.insert(manager.get(createApiRequest(course, "viewAllMediaLibraries")),
        {course, ReplyInfo::files});
    }

    if(options->isAnnouncementAttachmentsCheckBoxChecked() && activeFeatures.contains("Announcements"))
    {
        replies.insert(manager.get(createApiRequest(course, "viewAllAnnouncements")),
        {course, ReplyInfo::files});
    }

    if(options->isEmailAttachmentsCheckBoxChecked() && activeFeatures.contains("Emails"))
    {
        replies.insert(manager.get(createApiRequest(course, "viewAllEmails")),
        {course, ReplyInfo::files});
    }
}

void L2pItemModel::addFilesFromReply(QNetworkReply *reply, Structureelement *course)
{
    QLOG_DEBUG() << tr("Dateiinformationen empfangen: ") << reply->url().toString();

    // Prüfen auf Fehler
    if (!reply->error())
    {
        Parser::parseFiles(reply, course,
                           options->downloadFolderLineEditText());
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
            Utils::errorMessageBox(tr("Beim Abruf des Inhalts einer Veranstaltung ist ein Fehler aufgetreten"), reply->errorString() % ";\n " % replyMessage);
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

        QList<Structureelement*> oldItems;
        if(oldData)
        {
            root = oldData->invisibleRootItem();
            for( int i=0; i < root->rowCount(); i++)
            {
                getItemList(static_cast<Structureelement*>(root->child(i)), oldItems);
            }

            foreach(Structureelement *item, items)
            {
                for(QList<Structureelement*>::iterator it = oldItems.begin(); it != oldItems.end(); it++)
                {
                    Structureelement *oldItem = *it;
                    if(item->data(urlRole) == oldItem->data(urlRole) && item->text() == oldItem->text())
                    {
                        item->setData(oldItem->data(includeRole), includeRole);
                        oldItems.erase(it);
                        break;
                    }
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

    QString url = apiUrl % apiCommand % access % cid;
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

    switch (replyInfo.type)
    {
    case ReplyInfo::courses:
    {
        addCoursesFromReply(reply);
        break;
    }
    case ReplyInfo::features:
    {
        addFeatureFromReply(reply, replyInfo.item);
        break;
    }
    case ReplyInfo::files:
    {
        addFilesFromReply(reply, replyInfo.item);
        break;
    }
    default:
    {
        QLOG_ERROR() << tr("Serverantwort wurde unbekannter Typ zugeordnet") % ":" % reply->url().toString();
    }
    }

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
