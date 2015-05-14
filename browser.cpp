#include "browser.h"
#include "ui_browser.h"

#include "options.h"

#include <QThread>
#include <QTextCodec>

#include <QStandardPaths>

#include "qslog/QsLog.h"

// Hauptadresse des Sharepointdienstes
QString MainURL = "https://www3.elearning.rwth-aachen.de";

Browser::Browser(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Browser)
{
    ui->setupUi(this);

    // Hinzufügen der Daten zur Baumansicht
    itemModel = new QStandardItemModel();
    proxyModel.setDynamicSortFilter(true);
    proxyModel.setSourceModel(itemModel);
    ui->dataTreeView->setModel(&proxyModel);

    // Erzeugen des NetzwerkAccessManagers
    manager = new QNetworkAccessManager(qApp);

    QObject::connect(ui->searchLineEdit, SIGNAL(returnPressed()), ui->searchPushButton, SLOT(click()));

    refreshCounter = 0;

    setupSignalsSlots();
}

Browser::~Browser()
{
    delete ui;
}

void Browser::init(Options *options)
{
    this->options = options;
}

void Browser::loadStructureelementFromXml(QDomElement item, QStandardItem *parentItem)
{
    if(item.isNull())
    {
        return;
    }

    QStandardItem *newChild;
    if(item.tagName() == "item")
    {
        QString name = item.attribute("name", "");
        QUrl url = QUrl(item.attribute("url", ""));
        int time = item.attribute("time", "0").toInt();
        int size = item.attribute("size", "0").toInt();
        QString cid = item.attribute("cid", "");
        MyItemType type = static_cast<MyItemType>(item.attribute("type", "").toInt());
        bool included = item.attribute("included", "0").toInt();

        newChild = new Structureelement(name, url, time, size, cid, type);
        newChild->setData(included, includeRole);

        parentItem->appendRow(newChild);
    }
    else if(item.tagName() == "root")
    {
        newChild = parentItem;
    }

    // Alle Kindknoten hinzufügen
    QDomNodeList children = item.childNodes();
    for(int i=0; i < children.length(); i++)
    {
        loadStructureelementFromXml(children.item(i).toElement(), newChild);
    }
}

void Browser::loadSettings()
{
    QSettings settings;

    settings.beginGroup("sizeFilter");
    ui->sizeLimitCheckBox->setChecked(settings.value("sizeLimit", false).toBool());
    ui->sizeLimitSpinBox->setValue(settings.value("sizeLimitValue", 10).toInt());
    settings.endGroup();

    settings.beginGroup("dateFilter");
    ui->dateFilterCheckBox->setChecked(settings.value("dateFilter", false).toBool());
    ui->minDateEdit->setDate(settings.value("minDate", QDate(2000, 1, 1)).toDate());
    ui->maxDateEdit->setDate(settings.value("maxDate", QDate(2042, 1, 1)).toDate());
    settings.endGroup();

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf-8"));

#if QT_VERSION >= 0x050400
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
#else
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#endif
    QLOG_DEBUG() << tr("Vermuteter Pfad der Progammdaten: ") << dataPath;

    QDir dir(dataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QFile file(dataPath + "/data.xml");
    if(!file.open(QIODevice::ReadWrite))
    {
        QLOG_ERROR() << tr("Kann keine Daten von Festplatte laden") << ": " << file.errorString();
        return;
    }

    QTextStream ts(&file);

    if(ts.atEnd())
    {
        QLOG_INFO() << tr("Keine Dateiliste auf der Festplatte gefunden.");
        return;
    }

    QDomDocument domDoc;
    QString errorMessage;
    if(!domDoc.setContent(ts.readAll(), &errorMessage))
    {
        QLOG_ERROR() << tr("Kann Daten von Festplatte nicht parsen: ") << errorMessage;
        return;
    }
    file.close();

    QDomElement root = domDoc.documentElement();
    loadStructureelementFromXml(root, itemModel->invisibleRootItem());

    QLinkedList<Structureelement*> items;
    getStructureelementsList(itemModel->invisibleRootItem(), items);
    Utils::checkAllFilesIfSynchronised(items, options->downloadFolderLineEditText());
}

void Browser::saveSettings()
{
    // Speichern aller Einstellungen
    QSettings settings;

    settings.beginGroup("sizeFilter");
    settings.setValue("sizeLimit",      ui->sizeLimitCheckBox->isChecked());
    settings.setValue("sizeLimitValue", ui->sizeLimitSpinBox->value());
    settings.endGroup();

    settings.beginGroup("dateFilter");
    settings.setValue("dateFilter",     ui->dateFilterCheckBox->isChecked());
    settings.setValue("mindate",        ui->minDateEdit->date());
    settings.setValue("maxDate",        ui->maxDateEdit->date());
    settings.endGroup();

    QDomDocument domDoc;
    saveStructureelementToXml(domDoc, itemModel->invisibleRootItem(), NULL);

#if QT_VERSION >= 0x050400
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
#else
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#endif
    QFile file(dataPath + "/data.xml");
    if(!file.open(QIODevice::WriteOnly))
    {
        return;
    }
    QTextStream ts(&file);
    ts << domDoc.toByteArray();
    file.close();
}

void Browser::downloadDirectoryLineEditChangedSlot(QString downloadDirectory)
{
    if (downloadDirectory.isEmpty())
    {
        ui->openDownloadfolderPushButton->setEnabled(false);
    }
    else
    {
        ui->openDownloadfolderPushButton->setEnabled(true);
    }
}

// Starten des Aktualisierungsvorgang durch das Abrufen der Veranstaltungen
void Browser::on_refreshPushButton_clicked()
{
    refreshCounter++;

    oldItemModel = itemModel;

    itemModel = new QStandardItemModel();
    proxyModel.setSourceModel(itemModel);

    // Zurücksetzen der freigeschalteten Schaltflächen
    updateButtons();

    // Einfrieren der Anzeige
    emit enableSignal(false);

    // Verbinden des Managers
    QObject::connect(manager, SIGNAL(finished(QNetworkReply *)),
                     this, SLOT(coursesRecieved(QNetworkReply *)));

    QLOG_DEBUG() << tr("Veranstaltungsrequest");

    QNetworkRequest request(QUrl("https://www3.elearning.rwth-aachen.de/_vti_bin/L2PServices/api.svc/v1/viewAllCourseInfo?accessToken=" % options->getAccessToken()));

    // Starten der Anfrage für die Veranstaltungen
    manager->get(request);
}

// Auslesen der empfangenen Semesterveranstaltungsnamen
void Browser::coursesRecieved(QNetworkReply *reply)
{

    QLOG_DEBUG() << tr("Veranstaltungen empfangen");
    // Prüfen auf Fehler beim Abruf
    if (!reply->error())
    {
        Parser::parseCourses(reply, itemModel);
    }
    else
    {
        Utils::errorMessageBox(tr("Beim Abruf der Veranstaltungen ist ein Fehler aufgetreten"), reply->errorString() % ";\n " % reply->readAll());
    }

    // Veranstaltungen alphabetisch sortieren
    itemModel->sort(0);

    QObject::disconnect(manager,
                        SIGNAL(finished(QNetworkReply *)),
                        this,
                        SLOT(coursesRecieved(QNetworkReply *)));

    // Aufruf der Funktion zur Aktualisierung der Dateien
    if(itemModel->rowCount() != 0)
    {
        requestFileInformation();
    }
    else
    {
        emit enableSignal(true);
        updateButtons();
    }
}

/// Anfordern der Dateien für jede Veranstaltung
void Browser::requestFileInformation()
{
    // Prüfen, ob überhaupt Dokumentorte ausgewählt wurden
    if (!options->isLearningMaterialsCheckBoxChecked()
        && !options->isSharedLearningmaterialsCheckBoxChecked()
        && !options->isAssignmentsCheckBoxChecked()
        && !options->isMediaLibrarysCheckBoxChecked()
        && !options->isEmailAttachmentsCheckBoxChecked()
        && !options->isAnnouncementAttachmentsCheckBoxChecked())
    {
        // Freischalten von Schaltflächen
        emit enableSignal(true);
        updateButtons();
        return;
    }

    QObject::connect(manager,
                     SIGNAL(finished(QNetworkReply *)),
                     this,
                     SLOT(filesRecieved(QNetworkReply *)));

    QList<Structureelement*> courses = Utils::getAllCourseItems(itemModel);

    // Sonderfall: Es existieren keine Veranstaltungen
    if (courses.size() == 0)
    {
        QObject::disconnect(manager, SIGNAL(finished(QNetworkReply *)),
                            this, SLOT(filesRecieved(QNetworkReply *)));
        // Freischalten von Schaltflächen
        updateButtons();
        emit enableSignal(true);
        return;
    }

    sslSecureChannelBugOccured = false;


    //Anfordern aller Daten per APIRequest
    foreach(Structureelement* course, courses)
    {
        // Löschen aller Dateien
        if (course->rowCount() > 0)
        {
            course->removeRows(0, course->rowCount());
        }

        // Ausführen des Requests für "Dokumente"
        if (options->isLearningMaterialsCheckBoxChecked())
        {
            QNetworkRequest *request = apiRequest(course, "viewAllLearningMaterials");

            // Einfügen und Absenden des Requests
            replies.insert(manager->get(*request),
                           course);

            delete request;
        }

        // Ausführen des Requests für "Strukturierte Materialien"
        if (options->isSharedLearningmaterialsCheckBoxChecked())
        {
            QNetworkRequest *request = apiRequest(course, "viewAllSharedDocuments");

            // Einfügen und Absenden des Requests
            replies.insert(manager->get(*request),
                           course);

            delete request;
        }

        // Ausführen des Requests für "Übungsbetrieb"
        if (options->isAssignmentsCheckBoxChecked())
        {
            QNetworkRequest *request = apiRequest(course, "viewAllAssignments");

            // Einfügen und Absenden des Requests
            replies.insert(manager->get(*request),
                           course);

            delete request;
        }

        // Ausführen des Requests für "Literatur"
        if (options->isMediaLibrarysCheckBoxChecked())
        {
            QNetworkRequest *request = apiRequest(course, "viewAllMediaLibraries");

            // Einfügen und Absenden des Requests
            replies.insert(manager->get(*request),
                           course);

            delete request;
        }

        // Ausführen des Requests für "Ankündigung Anhänge"
        if (options->isAnnouncementAttachmentsCheckBoxChecked())
        {
            QNetworkRequest *request = apiRequest(course, "viewAllAnnouncements");

            // Einfügen und Absenden des Requests
            replies.insert(manager->get(*request),
                           course);

            delete request;
        }

        // Ausführen des Requests für "E-Mail Anhänge"
        if (options->isEmailAttachmentsCheckBoxChecked())
        {
            QNetworkRequest *request = apiRequest(course, "viewAllEmails");

            // Einfügen und Absenden des Requests
            replies.insert(manager->get(*request),
                           course);

            delete request;
        }

    }
}

void Browser::filesRecieved(QNetworkReply *reply)
{
    QLOG_DEBUG() << tr("Itemrequest empfangen: ") << reply->url().toString();

    // Prüfen auf Fehler
    if (!reply->error())
    {
        Parser::parseFiles(reply, &replies,
                           options->downloadFolderLineEditText());
    }
    else
    {
        QString replyMessage(reply->readAll());

         if(replyMessage.contains("secure channel"))
        {
            sslSecureChannelBugOccured = true;
            QLOG_DEBUG() << tr("SSL Fehler für: ") << reply->url().toString();
        }
        else if (replyMessage.contains("Assignment-Module is deactivated"))
        {
            QLOG_DEBUG() << tr("Assignment-Module ist deaktiviert für: ") << reply->url().toString();
        }
        else
        {
            Utils::errorMessageBox(tr("Beim Abruf des Inhalts einer Veranstaltung ist ein Fehler aufgetreten"), reply->errorString() % ";\n " % replyMessage);
        }
    }

    // Löschen der Antwort aus der Liste der abzuarbeitenden Antworten
    replies.remove(reply);
    // Freigabe des Speichers
    reply->deleteLater();

    // Prüfen, ob alle Antworten bearbeitet wurden
    if (replies.empty())
    {
        if(sslSecureChannelBugOccured)
        {
            Utils::errorMessageBox(tr("Beim Abruf des Inhalts mindestens einer Veranstaltung ist ein Fehler aufgetreten"),
                                   tr("Es können einige Dateien fehlen. Dieser Fehler wird nicht durch Sync-my-L2P verschuldet und ist bekannt."
                                      " Klicke erneut auf Aktualisieren bis dieser Fehler nicht mehr auftaucht."));
        }

        QObject::disconnect(manager, SIGNAL(finished(QNetworkReply *)),
                            this,    SLOT(filesRecieved(QNetworkReply *)));

        itemModel->sort(0);

        QLinkedList<Structureelement*> items;

        QStandardItem* root = itemModel->invisibleRootItem();
        for( int i=0; i < root->rowCount(); i++)
        {
            getStructureelementsList(static_cast<Structureelement*>(root->child(i)), items);
        }

        QLinkedList<Structureelement*> oldItems;
        root = oldItemModel->invisibleRootItem();
        for( int i=0; i < root->rowCount(); i++)
        {
            getStructureelementsList(static_cast<Structureelement*>(root->child(i)), oldItems);
        }

        foreach(Structureelement *item, items)
        {
            for(QLinkedList<Structureelement*>::iterator it = oldItems.begin(); it != oldItems.end(); it++)
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

        oldItemModel->deleteLater();
        oldItemModel = 0;

        Utils::checkAllFilesIfSynchronised(items, options->downloadFolderLineEditText());

        // Freischalten von Schaltflächen
        updateButtons();

        // Anzeigen aller neuen, unsynchronisierten Dateien
        if (refreshCounter == 1)
        {
            on_showNewDataPushButton_clicked();
        }

        emit enableSignal(true);

        // Automatische Synchronisation beim Programmstart
        if (options->isAutoSyncOnStartCheckBoxChecked() && refreshCounter==1 && options->getLoginCounter()==1
                && !sslSecureChannelBugOccured)
        {
            on_syncPushButton_clicked();
        }
    }
}

void Browser::on_syncPushButton_clicked()
{
    emit enableSignal(false);
    QString downloadPath = options->downloadFolderLineEditText();

    // Falls noch kein Downloadverzeichnis angegeben wurde, abbrechen
    if (downloadPath.isEmpty())
    {
        Utils::errorMessageBox(tr("Downloadverzeichnis fehlt!"), tr("Download unmöglich, da kein Zielverzeichnis angegeben wurde."));
        QLOG_ERROR() << tr("Kann nicht synchronisieren, da kein Downloadverzeichnis angegeben wurde");
        emit switchTab(1);
        emit enableSignal(true);
        return;
    }

    QDir verzeichnis(downloadPath);

    // Überprüfung, ob das angegebene Verzeichnis existiert oder
    // erstellt werden kann
    if (!verzeichnis.exists() && !verzeichnis.mkpath(verzeichnis.path()))
    {
        QLOG_ERROR() << tr("Kann Verzeichnis nicht erzeugen. Download abgebrochen.");
        emit enableSignal(true);
        return;
    }



    // Hinzufügen aller eingebundenen Elemente
    QLinkedList<Structureelement *> elementList;

    for (int i = 0; i < itemModel->rowCount(); i++)
    {
        getStructureelementsList((Structureelement *) itemModel->item(i, 0), elementList, true);
    }

    // Abbruch bei fehlenden Elementen
    if (elementList.isEmpty())
    {
        emit enableSignal(true);
        return;
    }

    int counter = getFileCount(elementList);
    FileDownloader *loader = new FileDownloader(
            counter,
            this);

    // Iterieren über alle Elemente
    int changedCounter = 0;
    QString courseName;

    QItemSelection newSelection;
    ui->dataTreeView->collapseAll();

    foreach(Structureelement *currentElement, elementList)
    {
        if(currentElement->type() != fileItem)
        {
            continue;
        }

        Structureelement* course = Utils::getParentCourse(currentElement);
        if(course)
        {
            courseName = course->text();
        }

        QString directoryPath = Utils::getElementLocalPath(currentElement, downloadPath, false, false);
        QDir directory(directoryPath);

        // Ordner ggf. erstellen
        if(!directory.mkpath(directoryPath))
        {
            Utils::errorMessageBox(tr("Verzeichnis nicht erstellbar!"), tr("Kann folgendes Verzeichnis nicht erstellen: ") + directoryPath);
            QLOG_ERROR() << tr("Verzeichnis nicht erstellbar: ") << directoryPath;
            break;
        }

        QString filename = currentElement->text();

        bool downloadFile = options->isOverrideFilesCheckBoxChecked() &&
                QFileInfo(directory, filename).lastModified().toMSecsSinceEpoch()/1000 < currentElement->data(dateRole).toDateTime().toMSecsSinceEpoch()/1000;
        downloadFile = downloadFile || !directory.exists(filename);

        // Datei existiert noch nicht
        if(downloadFile)
        {
            QString url = QString("https://www3.elearning.rwth-aachen.de/_vti_bin/l2pservices/api.svc/v1/") %
                    QString("downloadFile/") %
                    currentElement->text() %
                    QString("?accessToken=") %
                    options->getAccessToken() %
                    QString("&cid=") %
                    currentElement->data(cidRole).toString() %
                    QString("&downloadUrl=") %
                    currentElement->data(urlRole).toUrl().toDisplayString(QUrl::FullyDecoded);

            if (!loader->startNextDownload(filename,
                                           courseName,
                                           directory.absoluteFilePath(filename),
                                           QUrl(QUrl::toPercentEncoding(url, ":/?=&")),
                                           changedCounter++,
                                           currentElement->data(sizeRole).toInt(),
                                           currentElement->data(dateRole).toDateTime().toMSecsSinceEpoch()/1000))
            {
                break;
            }

            currentElement->setData(NOW_SYNCHRONISED, synchronisedRole);
            ui->dataTreeView->scrollTo(proxyModel.mapFromSource(currentElement->index()));
            newSelection.select(currentElement->index(), currentElement->index());
        }
    }

#ifdef __linux__
    // Kurzes warten, da es sonst zu Fehldarstellungen unter Linux kommen kann,
    // wenn das Fenster zu schnell wieder geschlossen wird
    QThread::msleep(20);
#endif
    loader->hide();
    loader->close();

    // Automatisches Beenden nach der Synchronisation
    if (options->isAutoCloseAfterSyncCheckBoxChecked())
    {
        AutoCloseDialog autoClose;
        if(!autoClose.exec())
        {
            QCoreApplication::quit();
        }
    }

    // Information über abgeschlossene Synchronisation anzeigen
    QMessageBox messageBox(this);
    QTimer::singleShot(10000, &messageBox, SLOT(accept()));
    messageBox.setText
    (tr("Synchronisation mit dem L2P der RWTH Aachen abgeschlossen."));
    messageBox.setIcon(QMessageBox::NoIcon);
    messageBox.setInformativeText(QString
                                  (tr("Es wurden %1 von %2 eingebundenen Dateien synchronisiert.\n(Dieses Fenster schließt nach 10 Sek. automatisch.)")).arg
                                  (QString::number(changedCounter),
                                   QString::number(counter)));
    messageBox.setStandardButtons(QMessageBox::Ok);
    messageBox.exec();

    // Alle synchronisierten Elemente auswählen
    ui->dataTreeView->selectionModel()->
    select(proxyModel.mapSelectionFromSource(newSelection),
           QItemSelectionModel::ClearAndSelect);


    emit enableSignal(true);
}

// Diese Funktion nimmt die Eingabe im Suchfeld und zeigt alle Treffer
// an und markiert sie
void Browser::on_searchPushButton_clicked()
{
    // Kein Suchwort
    if (ui->searchLineEdit->text().isEmpty())
    {
        return;
    }

    // Alle Elemente mit dem Suchwort finden
    QList<QStandardItem *> found = itemModel->findItems("*" % ui->searchLineEdit->text() % "*",
                                                           Qt::MatchWildcard | Qt::MatchRecursive);

    // Gefundene Elemente auswählen und anzeigen
    QItemSelection newSelection;
    ui->dataTreeView->collapseAll();

    foreach(QStandardItem * item, found)
    {
        newSelection.select(item->index(), item->index());
        ui->dataTreeView->scrollTo(proxyModel.mapFromSource(item->index()));
    }

    ui->dataTreeView->selectionModel()->
    select(proxyModel.mapSelectionFromSource(newSelection),
           QItemSelectionModel::ClearAndSelect);
}

// Ausschließen der ausgewählten Elemente, ggf. deren Vaterelemente und alle ihre Kindelemente
void Browser::on_removeSelectionPushButton_clicked()
{
    // Holen der ausgewählten Dateien
    QModelIndexList selectedElementsIndexList = proxyModel.mapSelectionToSource(ui->dataTreeView->selectionModel()->selection()).indexes();
    // Iteration über alle Elemente
    Structureelement *parentElement = 0;
    QModelIndexList::Iterator iteratorEnd = selectedElementsIndexList.end();

    for (QModelIndexList::Iterator iterator = selectedElementsIndexList.begin();
         iterator != iteratorEnd; iterator++)
    {
        // Holen des Vaterelements des ausgewählten Strukturelements
        parentElement = (Structureelement *) iterator->internalPointer();

        // Ausschließen des ausgewählten Elements
        Structureelement *selectedElement = (Structureelement *) parentElement->child(iterator->row(), 0);

        // Ausschließen aller Kindelemente
        removeSelection(selectedElement);

        // Prüfen, ob alle Geschwisterelemente ausgeschlossen sind
        // Falls ja, ausschließen des Vaterlements und rekursiv wiederholen

        // Hinweis: Nur wenn selectedElement ein Toplevel-Item ist, kann parentElement invisibleRootItem sein
        // ist selectedElement ein tieferes Item, ist der oberste Parent 0x00
        while ((parentElement != 0) && (parentElement != itemModel->invisibleRootItem()))
        {
            bool siblingsExcluded = true;

            // Prüfung aller Zeilen, ob alle ausgeschlossen
            for (int i = 0; i < parentElement->rowCount(); i++)
            {
                if (((Structureelement *) parentElement->
                     child(i, 0))->data(includeRole).toBool())
                    siblingsExcluded = false;
            }

            // Falls ja, Vaterelement auch ausschließen
            if (siblingsExcluded)
                parentElement->setData(false, includeRole);

            parentElement = (Structureelement*) parentElement->parent();

        }
    }

    // Aktualisieren der kompletten Ansicht
    ui->dataTreeView->dataChanged(itemModel->index(0, 0),
                                  itemModel->index(itemModel->rowCount(), 0));
}

// Rekursives ausschließen aller untergeordneten Elemente des übergebenen Elements
void Browser::removeSelection(Structureelement *element)
{
    if (element->data(includeRole).toBool())
    {
        element->setData(false, includeRole);

        for (int i = 0; i < element->rowCount(); i++)
        {
            removeSelection((Structureelement *) element->child(i, 0));
        }
    }
}

// Einbinden der ausgewählten Elemente, deren Vaterelemente und alle ihre Kindelemente
void Browser::on_addSelectionPushButton_clicked()
{
    // Bestimmen der ausgewählten Items
    QModelIndexList selectedElementsIndexList = proxyModel
            .mapSelectionToSource(ui->dataTreeView->selectionModel()->selection())
            .indexes();

    // Variablen zur Speicherung von Pointern aus Performancegründen vor
    // der Schleife
    Structureelement *element = 0;
    QModelIndexList::Iterator iteratorEnd = selectedElementsIndexList.end();

    for (QModelIndexList::Iterator iterator = selectedElementsIndexList.begin();
         iterator != iteratorEnd; iterator++)
    {
        // Holen des Pointers auf das ausgewählte Item
        // Hinweis: internalPointer liefert einen Pointer auf das Vaterelement
        element = (Structureelement *) ((Structureelement *) iterator->internalPointer())->child(iterator->row(), 0);

        // Einbinden aller übergeordneter Ordner
        Structureelement *parent = element;

        while ((parent = (Structureelement *) parent->parent()) != 0)
        {
            // Annahme: Wenn ein übergeordnetes Element eingebunden ist, dann sind es auch alle darüber
            if (parent->data(includeRole).toBool())
                break;

            parent->setData(true, includeRole);
        }

        // Einbinden aller untergeordneter Ordner und Dateien
        addSelection(element);
    }

    // Aktualisierung der kompletten Ansicht
    ui->dataTreeView->dataChanged(itemModel->index(0, 0), itemModel->index(itemModel->rowCount(), 0));
}

// Rekursives Durchlaufen alle Kindelemente eines Elements und Einbindung dieser
void Browser::addSelection(Structureelement *aktuelleStruktur)
{
    // Einbinden des aktuellen Items
    aktuelleStruktur->setData(true, includeRole);
    // Einbinden aller untergeordnete Ordner und Dateien
    int rowCount = aktuelleStruktur->rowCount();

    for (int i = 0; i < rowCount; i++)
        addSelection((Structureelement *) aktuelleStruktur->child(i, 0));
}

void Browser::getStructureelementsList(Structureelement *currentElement, QLinkedList <Structureelement *> &liste, bool onlyIncluded)
{
    if (!onlyIncluded
        || (onlyIncluded && currentElement->data(includeRole).toBool()))
    {
        if (((ui->sizeLimitCheckBox->isChecked()
             && currentElement->data(sizeRole).toInt() <
             (ui->sizeLimitSpinBox->value() * 1024 * 1024))
            || !ui->sizeLimitCheckBox->isChecked())
                && ((ui->dateFilterCheckBox->isChecked() && ((ui->minDateEdit->date() <= currentElement->data(dateRole).toDate() && ui->maxDateEdit->date() >= currentElement->data(dateRole).toDate())|| !currentElement->data(dateRole).toDate().isValid())) || !ui->dateFilterCheckBox->isChecked()))
        liste.append(currentElement);


        if (currentElement->hasChildren())
            for (int i = 0; i < currentElement->rowCount(); i++)
                getStructureelementsList((Structureelement *)
                                         currentElement->child(i, 0),
                                         liste, onlyIncluded);
    }
}

void Browser::getStructureelementsList(QStandardItem *topElement, QLinkedList <Structureelement *> &list)
{
    Structureelement *item = dynamic_cast<Structureelement*>(topElement);
    if(item)
    {
        list.append(item);
    }
    for (int i = 0; i < topElement->rowCount(); i++)
        getStructureelementsList(topElement->child(i), list);
}

/// Bestimmung der Zahl der Dateien in einer Liste
int Browser::getFileCount(QLinkedList < Structureelement * > &items)
{
    int fileCounter = 0;

    foreach(Structureelement* item, items)
    {
        if(item->type() == fileItem)
        {
            fileCounter++;
        }
    }

    return fileCounter;
}

void Browser::saveStructureelementToXml(QDomDocument &domDoc, QStandardItem *item, QDomElement *parentItem)
{
    QDomElement xmlItem;

    // Das Root-item auslassen
    if(item->text().isEmpty())
    {
        xmlItem = domDoc.createElement("root");
        domDoc.appendChild(xmlItem);
    }
    else
    {
        xmlItem = domDoc.createElement("item");
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
        saveStructureelementToXml(domDoc, item->child(i), &xmlItem);
    }
}



// Aktivierung oder Deaktivierung der Buttons in Abhängigkeit des Status
void Browser::updateButtons()
{
    if(itemModel->rowCount() == 0 || options->getAccessToken().isEmpty())
    {
        ui->showNewDataPushButton->setEnabled(false);
        ui->expandPushButton->setEnabled(false);
        ui->contractPushButton->setEnabled(false);

        ui->removeSelectionPushButton->setEnabled(false);
        ui->addSelectionPushButton->setEnabled(false);
        ui->syncPushButton->setEnabled(false);
    }
    else
    {
        ui->showNewDataPushButton->setEnabled(true);
        ui->expandPushButton->setEnabled(true);
        ui->contractPushButton->setEnabled(true);

        ui->removeSelectionPushButton->setEnabled(true);
        ui->addSelectionPushButton->setEnabled(true);
        ui->syncPushButton->setEnabled(true);
    }

    ui->refreshPushButton->setEnabled(!options->getAccessToken().isEmpty());
}

void Browser::setupSignalsSlots()
{
    connect(ui->expandPushButton, &QPushButton::clicked, ui->dataTreeView, &QTreeView::expandAll);
    connect(ui->contractPushButton, &QPushButton::clicked, ui->dataTreeView, &QTreeView::collapseAll);

    connect(ui->sizeLimitSpinBox, SIGNAL(valueChanged(int)), &proxyModel, SLOT(setMaximumSize(int)));
    connect(ui->sizeLimitCheckBox, &QCheckBox::toggled, &proxyModel, &MySortFilterProxyModel::setMaximumSizeFilter);

    connect(ui->dateFilterCheckBox, &QCheckBox::toggled, &proxyModel, &MySortFilterProxyModel::setInRangeDateFilter);
    connect(ui->minDateEdit, &QDateEdit::dateChanged, &proxyModel, &MySortFilterProxyModel::setFilterMinimumDate);
    connect(ui->maxDateEdit, &QDateEdit::dateChanged, &proxyModel, &MySortFilterProxyModel::setFilterMaximumDate);
}

QNetworkRequest *Browser::apiRequest(Structureelement *course, QString apiExtension)
{
    QString baseUrl = "https://www3.elearning.rwth-aachen.de/_vti_bin/L2PServices/api.svc/v1/";
    QString access = "?accessToken=" % options->getAccessToken();
    QString cid = "&cid=" % course->data(cidRole).toString();

    QString url = baseUrl % apiExtension % access % cid;
    QNetworkRequest *request = new QNetworkRequest(QUrl(QUrl::toPercentEncoding(url, ":/?=&")));

    QSslConfiguration conf = request->sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request->setSslConfiguration(conf);

    QLOG_DEBUG() << tr("Itemrequest an API: ") << url;

    // Damit der L2P Server nicht überfordert wird...
    QThread::msleep(10);

    return request;
}

void Browser::on_openDownloadfolderPushButton_clicked()
{
    // Betriebssystem soll mit dem Standardprogramm den Pfad öffnen
    QDesktopServices::openUrl(QUrl
                              ("file:///" %
                               options->downloadFolderLineEditText(),
                               QUrl::TolerantMode));
}

/// Dateien bei einem Doppelklick öffnen
void Browser::on_dataTreeView_doubleClicked(const QModelIndex &index)
{
    Structureelement *item =
        (Structureelement *) itemModel->
        itemFromIndex(proxyModel.mapToSource(index));

    if (item->type() == fileItem)
    {
        QFileInfo fileInfo(Utils::getElementLocalPath(item,
                                                      options->downloadFolderLineEditText(),
                                                      true,
                                                      false));

        QString fileUrl;

        // Überprüfung, ob Datei lokal oder im L2P geöffnet werden soll
        if(fileInfo.exists() && fileInfo.isFile())
        {
            fileUrl = Utils::getElementLocalPath(item, options->downloadFolderLineEditText());
        }
        else
        {
            fileUrl = Utils::getElementRemotePath(item, "https://www3.elearning.rwth-aachen.de/");
        }

        QDesktopServices::openUrl(QUrl(fileUrl));
    }
}

void Browser::on_dataTreeView_customContextMenuRequested(const QPoint &pos)
{
    // Bestimmung des Elements, auf das geklickt wurde
    Structureelement *RightClickedItem =
        (Structureelement *) itemModel->
        itemFromIndex(proxyModel.mapToSource(ui->dataTreeView->indexAt(pos)));

    // Überprüfung, ob überhaupt auf ein Element geklickt wurde (oder
    // ins Leere)
    if (RightClickedItem == 0)
    {
        return;
    }

    // Speichern des geklickten Items
    lastRightClickItem = RightClickedItem;
    // Erstellen eines neuen Menus
    QMenu newCustomContextMenu(this);

    // Öffnen der Veranstaltungsseite im L2P
    if (RightClickedItem->type() == courseItem)
    {
        newCustomContextMenu.addAction(tr("Veranstaltungsseite öffnen"), this, SLOT(openCourse()));
    }

    // Öffnen des Elements lokal oder im L2P
    newCustomContextMenu.addAction(tr("Öffnen"), this, SLOT(openFile()));

    // Kopieren der URL
    if(RightClickedItem->type() == courseItem || RightClickedItem->type() == fileItem)
    {
        newCustomContextMenu.addAction(tr("Link kopieren"), this, SLOT(copyUrlToClipboardSlot()));
    }

    // Anzeigen des Menus an der Mausposition
    newCustomContextMenu.exec(ui->dataTreeView->mapToGlobal(pos));
}

void Browser::openCourse()
{
    // Öffnen der URL des mit der rechten Maustaste geklickten Items
    QDesktopServices::openUrl(lastRightClickItem->data(urlRole).toUrl());
}

void Browser::openFile()
{
    QString baseUrl = "https://www3.elearning.rwth-aachen.de";

    QFileInfo fileInfo(Utils::getElementLocalPath(lastRightClickItem,
                                                  options->downloadFolderLineEditText(),
                                                  true,
                                                  false));

    QString fileUrl;

    // Überprüfung, ob Datei lokal oder im L2P geöffnet werden soll
    if(fileInfo.exists())
    {
        fileUrl = Utils::getElementLocalPath(lastRightClickItem, options->downloadFolderLineEditText());
    }
    else
    {
        fileUrl = Utils::getElementRemotePath(lastRightClickItem, baseUrl);
    }

    QDesktopServices::openUrl(QUrl(fileUrl));
}

void Browser::on_showNewDataPushButton_clicked()
{
    QLinkedList<Structureelement*> list;

    for (int i = 0; i < itemModel->rowCount(); i++)
        getStructureelementsList((Structureelement*)(itemModel->invisibleRootItem()->child(i)), list);

    QItemSelection newSelection;
    ui->dataTreeView->collapseAll();

    foreach(Structureelement * item, list)
    {
        if ((item->type() == fileItem) && (item->data(synchronisedRole) == NOT_SYNCHRONISED))
        {
            newSelection.select(item->index(), item->index());
            ui->dataTreeView->scrollTo(proxyModel.mapFromSource(item->index()));
        }
    }

    ui->dataTreeView->selectionModel()->
            select(proxyModel.mapSelectionFromSource(newSelection),
                   QItemSelectionModel::ClearAndSelect);
}

void Browser::copyUrlToClipboardSlot()
{
    QString url;
    if(lastRightClickItem->type() == fileItem)
    {
        url = Utils::getElementRemotePath(lastRightClickItem, "https://www3.elearning.rwth-aachen.de");
    }
    else if(lastRightClickItem->type() == courseItem)
    {
        url = lastRightClickItem->data(urlRole).toString();
    }

    Utils::copyTextToClipboard(url);
}

void Browser::successfulLoginSlot()
{
    ui->refreshPushButton->setEnabled(true);
}

void Browser::clearItemModel()
{
    itemModel->clear();
    updateButtons();
}

void Browser::retranslate()
{
    ui->retranslateUi(this);
}
