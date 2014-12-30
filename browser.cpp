#include "browser.h"
#include "ui_browser.h"

#include "options.h"
#include <QDebug>

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
#include <iostream>
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

    QFile file("data.xml");
    if(!file.open(QIODevice::ReadWrite))
    {
        QLOG_ERROR() << "Kann keine Daten von Festplatte laden.";
        return;
    }
    QTextStream ts(&file);

    QDomDocument domDoc;
    QString errorMessage;
    if(!domDoc.setContent(ts.readAll(), &errorMessage))
    {
        QLOG_ERROR() << "Kann Daten von Festplatte nicht parsen: " << errorMessage;
        return;
    }
    file.close();

    QDomElement root = domDoc.documentElement();
    loadStructureelementFromXml(root, itemModel->invisibleRootItem());
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

    QFile file("data.xml");
    if(!file.open(QIODevice::WriteOnly))
    {
        return;
    }
    QTextStream ts(&file);
    ts << domDoc.toString();
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

    QLOG_INFO() << "Veranstaltungsrequest";

    QNetworkRequest request(QUrl("https://www3.elearning.rwth-aachen.de/_vti_bin/L2PServices/api.svc/v1/viewAllCourseInfo?accessToken=" % options->getAccessToken()));

    // Starten der Anfrage für die Veranstaltungen
    manager->get(request);
}

// Auslesen der empfangenen Semesterveranstaltungsnamen
void Browser::coursesRecieved(QNetworkReply *reply)
{

    QLOG_INFO() << "Veranstaltungen empfangen";
    // Prüfen auf Fehler beim Abruf
    if (!reply->error())
    {
        Parser::parseCourses(reply, itemModel);
    }
    else
    {
        Utils::errorMessageBox("Beim Abruf der Veranstaltungen ist ein Fehler aufgetreten", reply->errorString());
        QLOG_ERROR() << "Veranstaltungen nicht abrufbar: " << reply->errorString();
    }

    // Veranstaltungen alphabetisch sortieren
    itemModel->sort(0);

    QObject::disconnect(manager,
                        SIGNAL(finished(QNetworkReply *)),
                        this,
                        SLOT(coursesRecieved(QNetworkReply *)));

    // Aufruf der Funktion zur Aktualisierung der Dateien
    requestFileInformation();
}

/// Anfordern der Dateien für jede Veranstaltung
void Browser::requestFileInformation()
{
    // Prüfen, ob überhaupt Dokumentorte ausgewählt wurden
    if (!options->isLearningMaterialsCheckBoxChecked()
        && !options->isSharedLearningmaterialsCheckBoxChecked()
        && !options->isAssignmentsCheckBoxChecked()
        && !options->isMediaLibrarysCheckBoxChecked())
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
//            replies.insert(manager->get(*request),
//                           course);

            delete request;
        }

        // Ausführen des Requests für "Literatur"
        if (options->isMediaLibrarysCheckBoxChecked())
        {
            QNetworkRequest *request = apiRequest(course, "viewAllMediaLibrarys");

            // Einfügen und Absenden des Requests
            replies.insert(manager->get(*request),
                           course);

            delete request;
        }
    }
}

void Browser::filesRecieved(QNetworkReply *reply)
{
    // Prüfen auf Fehler
    if (!reply->error())
    {
        Parser::parseFiles(reply, &replies,
                           options->downloadFolderLineEditText());
    }
    else
    {
        Utils::errorMessageBox("Beim Abruf des Inhalts einer Veranstaltung ist ein Fehler aufgetreten", reply->errorString());
        QLOG_ERROR() << "Fehler beim Abrufen der Dateien einer Veranstaltung: " << reply->errorString();
        QLOG_ERROR() << reply->readAll();
    }

    // Löschen der Antwort aus der Liste der abzuarbeitenden Antworten
    replies.remove(reply);
    // Freigabe des Speichers
    reply->deleteLater();

    // Prüfen, ob alle Antworten bearbeitet wurden
    if (replies.empty())
    {
        QObject::disconnect(manager, SIGNAL(finished(QNetworkReply *)),
                            this,    SLOT(filesRecieved(QNetworkReply *)));

        // Freischalten von Schaltflächen
        updateButtons();

        // Anzeigen aller neuen, unsynchronisierten Dateien
        if (refreshCounter == 1)
        {
            on_showNewDataPushButton_clicked();
        }

        emit enableSignal(true);

        // Automatische Synchronisation beim Programmstart
        if (options->isAutoSyncOnStartCheckBoxChecked() && refreshCounter==1 && options->getLoginCounter()==1)
        {
            on_syncPushButton_clicked();
        }

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
    }
}

void Browser::on_syncPushButton_clicked()
{
    emit enableSignal(false);
    QString downloadPath = options->downloadFolderLineEditText();

    // Falls noch kein Downloadverzeichnis angegeben wurde, abbrechen
    if (downloadPath.isEmpty())
    {
        Utils::errorMessageBox("Downloadverzeichnis fehlt!", "Download unmöglich, da kein Zielverzeichnis angegeben wurde.");
        QLOG_ERROR() << "Kann nicht synchronisieren, da kein Downloadverzeichnis angegeben wurde";
        emit switchTab(1);
        emit enableSignal(true);
        return;
    }

    QDir verzeichnis(downloadPath);

    // Überprüfung, ob das angegebene Verzeichnis existiert oder
    // erstellt werden kann
    if (!verzeichnis.exists() && !verzeichnis.mkpath(verzeichnis.path()))
    {
        QLOG_ERROR() << "Kann Verzeichnis nicht erzeugen. Download abgebrochen.";
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
            options->isOriginalModifiedDateCheckBoxChecked(),
            this);

    // Iterieren über alle Elemente
    int changedCounter = 0;
    QString veranstaltungName;

    QItemSelection newSelection;
    ui->dataTreeView->collapseAll();

    foreach(Structureelement *currentElement, elementList)
    {
        if(currentElement->type() != fileItem)
        {
            continue;
        }

        QString directoryPath = Utils::getElementLocalPath(currentElement, downloadPath, false, false);
        QDir directory(directoryPath);

        // Ordner ggf. erstellen
        if(!directory.mkpath(directoryPath))
        {
            Utils::errorMessageBox("Verzeichnis nicht erstellbar!", "Kann folgendes Verzeichnis nicht erstellen: " + directoryPath);
            QLOG_ERROR() << "Verzeichnis nicht erstellbar: " << directoryPath;
            break;
        }

        QString filename = currentElement->text();

        // Datei existiert noch nicht
        if (!directory.exists(filename) ||
            (QFileInfo(directory, filename).size()
             != (currentElement->data(sizeRole).toInt())))
        {
            QString url = QString("https://www3.elearning.rwth-aachen.de/_vti_bin/l2pservices/api.svc/v1/") %
                    QString("downloadFile/") %
                    currentElement->text() %
                    QString("?accessToken=") %
                    options->getAccessToken() %
                    QString("&cid=") %
                    currentElement->data(cidRole).toString() %
                    QString("&downloadUrl=") %
                    currentElement->data(urlRole).toString();

            if (!loader->startNextDownload(filename,
                                           veranstaltungName,
                                           directory.absoluteFilePath(filename),
                                           QUrl(url),
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
    ("Synchronisation mit dem L2P der RWTH Aachen abgeschlossen.");
    messageBox.setIcon(QMessageBox::NoIcon);
    messageBox.setInformativeText(QString
                                  ("Es wurden %1 von %2 eingebundenen Dateien synchronisiert.\n(Dieses Fenster schließt nach 10 Sek. automatisch.)").arg
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

void Browser::getStructureelementsList(Structureelement *topElement, QLinkedList <Structureelement *> &list)
{
    list.append(topElement);
    for (int i = 0; i < topElement->rowCount(); i++)
        getStructureelementsList((Structureelement*) topElement->child(i), list);
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
    if(itemModel->rowCount() == 0)
    {
        ui->refreshPushButton->setEnabled(false);
        ui->showNewDataPushButton->setEnabled(false);
        ui->expandPushButton->setEnabled(false);
        ui->contractPushButton->setEnabled(false);

        ui->sizeLimitCheckBox->setEnabled(false);
        ui->sizeLimitSpinBox->setEnabled(false);
        ui->dateFilterCheckBox->setEnabled(false);
        ui->minDateEdit->setEnabled(false);
        ui->maxDateEdit->setEnabled(false);

        ui->searchLineEdit->setEnabled(false);
        ui->searchPushButton->setEnabled(false);

        ui->removeSelectionPushButton->setEnabled(false);
        ui->addSelectionPushButton->setEnabled(false);
        ui->syncPushButton->setEnabled(false);
    }
    else
    {
        ui->refreshPushButton->setEnabled(true);
        ui->showNewDataPushButton->setEnabled(true);
        ui->expandPushButton->setEnabled(true);
        ui->contractPushButton->setEnabled(true);

        ui->sizeLimitCheckBox->setEnabled(true);
        ui->sizeLimitSpinBox->setEnabled(true);
        ui->dateFilterCheckBox->setEnabled(true);
        ui->minDateEdit->setEnabled(true);
        ui->maxDateEdit->setEnabled(true);

        ui->searchLineEdit->setEnabled(true);
        ui->searchPushButton->setEnabled(true);

        ui->removeSelectionPushButton->setEnabled(true);
        ui->addSelectionPushButton->setEnabled(true);
        ui->syncPushButton->setEnabled(true);
    }

}

QNetworkRequest *Browser::apiRequest(Structureelement *course, QString apiExtension)
{
    QString baseUrl = "https://www3.elearning.rwth-aachen.de/_vti_bin/L2PServices/api.svc/v1/";
    QString access = "?accessToken=" % options->getAccessToken();
    QString cid = "&cid=" % course->data(cidRole).toString();

    QString url = baseUrl % apiExtension % access % cid;
    QNetworkRequest *request = new QNetworkRequest(QUrl(url));

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

void Browser::on_expandPushButton_clicked()
{
    // Expandieren aller Zweige
    ui->dataTreeView->expandAll();
}

void Browser::on_contractPushButton_clicked()
{
    // Reduktion aller Zweige
    ui->dataTreeView->collapseAll();
}

void Browser::on_dataTreeView_doubleClicked(const QModelIndex &index)
{
    Structureelement *element =
        (Structureelement *) itemModel->
        itemFromIndex(proxyModel.mapToSource(index));

    if (element->type() == fileItem)
        if (!QDesktopServices::openUrl(QUrl
                                       (Utils::getElementLocalPath(element,
                                               options->downloadFolderLineEditText()),
                                        QUrl::TolerantMode)))
        {
            QDesktopServices::openUrl(element->data(urlRole).toUrl());
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
        return;

    // Speichern des geklickten Items
    lastRightClickItem = RightClickedItem;
    // Erstellen eines neuen Menus
    QMenu newCustomContextMenu(this);

    // Öffnen der Veranstaltungsseite im L2P
    if (RightClickedItem->type() == courseItem)
    {
        QAction *openCourseAction = new QAction("Veranstaltungsseite oeffnen", this);
        newCustomContextMenu.addAction(openCourseAction);
        QObject::connect(openCourseAction, SIGNAL(triggered()), this,
                         SLOT(openCourse()));
    }

    // Öffnen der Datei
    QAction *openAction = new QAction("Oeffnen", this);
    newCustomContextMenu.addAction(openAction);
    QObject::connect(openAction, SIGNAL(triggered()), this, SLOT(openItem()));
    // Kopieren der URL
    QAction *copyAction = new QAction("Link kopieren", this);
    newCustomContextMenu.addAction(copyAction);
    QObject::connect(copyAction, SIGNAL(triggered()), this,
                     SLOT(copyUrlToClipboardSlot()));
    // Anzeigen des Menus an der Mausposition
    newCustomContextMenu.exec(ui->dataTreeView->mapToGlobal(pos));
}

void Browser::openCourse()
{
    // Öffnen der URL des mit der rechten Maustaste geklickten Items
    QDesktopServices::openUrl(lastRightClickItem->data(urlRole).toUrl());
}

void Browser::openItem()
{
    QLOG_INFO() << Utils::getElementLocalPath(lastRightClickItem, options->downloadFolderLineEditText());
    // Öffnen der Datei auf der Festplatte des mit der rechten Maustaste
    // geklickten Items
    if (!QDesktopServices::openUrl(QUrl(Utils::getElementLocalPath(lastRightClickItem, options->downloadFolderLineEditText()), QUrl::TolerantMode)))
    {
        // Öffnen der Datei im L2P des mit der rechten Maustaste
        // geklickten Items
        QDesktopServices::openUrl(lastRightClickItem->data(urlRole).toUrl());
    }
}

void Browser::on_sizeLimitSpinBox_valueChanged(int newMaximumSize)
{
    proxyModel.setMaximumSize(newMaximumSize);
}

void Browser::on_sizeLimitCheckBox_toggled(bool checked)
{
    proxyModel.setMaximumSizeFilter(checked);
}

void Browser::on_dateFilterCheckBox_toggled(bool checked)
{
    proxyModel.setInRangeDateFilter(checked);
}

void Browser::on_minDateEdit_dateChanged(const QDate &date)
{
    proxyModel.setFilterMinimumDate(date);
}

void Browser::on_maxDateEdit_dateChanged(const QDate &date)
{
    proxyModel.setFilterMaximumDate(date);
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
    Utils::copyTextToClipboard(lastRightClickItem->text());
}

void Browser::successfulLoginSlot()
{
    ui->refreshPushButton->setEnabled(true);
}

