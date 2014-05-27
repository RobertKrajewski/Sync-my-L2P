#include "browser.h"
#include "ui_browser.h"

#include "options.h"
#include <QDebug>

// Hauptadresse des Sharepointdienstes
QString MainURL = "https://www2.elearning.rwth-aachen.de";

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
    QObject::connect(manager,
                     SIGNAL(authenticationRequired
                          (QNetworkReply *, QAuthenticator *)), this,
                     SLOT(doAuthentification
                          (QNetworkReply *, QAuthenticator *)));

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
    ui->maxDateEdit->setDate(settings.value("maxDate", QDate(2020, 1, 1)).toDate());
    settings.endGroup();
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
}

// Starten des Aktualisierungsvorgang durch das Abrufen der Semesterveranstaltungen
void Browser::on_refreshPushButton_clicked()
{
    refreshCounter++;

    // Löschen der alten Veranstaltungsliste
    itemModel->clear();

    // Zurücksetzen der freigeschalteten Schaltflächen
    updateButtons();

    // Einfrieren der Anzeige
    emit enableSignal(false);

    // Verbinden des Managers
    QObject::connect(manager, SIGNAL(finished(QNetworkReply *)),
                     this, SLOT(coursesRecieved(QNetworkReply *)));

    // Starten der Anfrage für das aktuelle Semester
    if (options->isCurrentSemesterCheckBoxChecked())
        replies.insert(manager->get(QNetworkRequest(QUrl(MainURL % "/foyer/summary/default.aspx"))), 0);

    // Starten einer Anfrage für ältere Semester
    if (options->isOldSemesterCheckBoxChecked())
        replies.insert(manager->get(QNetworkRequest(QUrl(MainURL % "/foyer/archive/default.aspx"))), 0);
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

// Auslesen der empfangenen Semesterveranstaltungsnamen
void Browser::coursesRecieved(QNetworkReply *reply)
{
    // Erst mit Daten im TreeView lässt sich der Header gestalten.
    QStringList headerLabels;
    headerLabels << "Name" << QString::fromUtf8("Größe") << "Datum";
    itemModel->setHorizontalHeaderLabels(headerLabels);

    #if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    ui->dataTreeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->dataTreeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->dataTreeView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    #else
    ui->dataTreeView->header()->setResizeMode(0, QHeaderView::Stretch);
    ui->dataTreeView->header()->setResizeMode(1, QHeaderView::ResizeToContents);
    ui->dataTreeView->header()->setResizeMode(2, QHeaderView::ResizeToContents);
    #endif

    // Prüfen auf Fehler beim Abruf
    if (!reply->error())
        Parser::parseCourses(reply, itemModel);
    else
        Utils::errorMessageBox("Beim Abruf der Veranstaltungen ist ein Fehler aufgetreten", reply->errorString());

    // Löschen der Antwort aus der Queue
    replies.remove(reply);
    // Antwort für das spätere Löschen markieren
    reply->deleteLater();

    // Prüfen, ob noch Antworten ausstehen und ggf. Reaktiveren der
    // Benutzeroberfläche
    if (replies.isEmpty())
    {
        // Veranstaltungen alphabetisch sortieren
        itemModel->sort(0);

        QObject::disconnect(manager,
                            SIGNAL(finished(QNetworkReply *)),
                            this,
                            SLOT(coursesRecieved(QNetworkReply *)));

        // Aufruf der Funktion zur Aktualisierung der Dateien
        getNewData();
    }
}

// Anfordern der Dokumentlisten für jeden Kurs
void Browser::getNewData()
{
    // Prüfen, ob überhaupt Dokumentorte ausgewählt wurden
    if (!options->isDocumentsCheckBoxChecked()
        && !options->isSharedMaterialsCheckBoxChecked()
        && !options->isExercisesCheckBoxChecked()
        && !options->isLiteratureCheckBoxChecked()
        && !options->isTutorDocumentsCheckBoxChecked())
    {
        // Freischalten von Schaltflächen
        emit enableSignal(true);
        return;
    }

    QObject::connect(manager,
                     SIGNAL(finished(QNetworkReply *)),
                     this,
                     SLOT(filesRecieved(QNetworkReply *)));

    // Durchlaufen aller Veranstaltungen
    int rowCount = itemModel->rowCount();

    // Sonderfall: Es existieren keine Veranstaltungen
    if (rowCount == 0)
    {
        QObject::disconnect(manager, SIGNAL(finished(QNetworkReply *)),
                            this, SLOT(filesRecieved(QNetworkReply *)));
        // Freischalten von Schaltflächen
        updateButtons();
        emit enableSignal(true);
        return;
    }

    //Anfordern aller Daten per WebDAV-Request
    for (int i = 0; i < rowCount; i++)
    {
        // Holen der aktuellen Veranstaltung
        Structureelement *aktuelleVeranstaltung = (Structureelement*) itemModel->item(i);

        // Löschen aller Dateien
        if (aktuelleVeranstaltung->rowCount() > 0)
            aktuelleVeranstaltung->removeRows(0, aktuelleVeranstaltung->rowCount());

        // Ausführen des Requests für "Dokumente"
        if (options->isDocumentsCheckBoxChecked())
        {
            QNetworkRequest *request = webdavRequest(aktuelleVeranstaltung, "/materials/documents/");

            // Einfügen und Absenden des Requests
            replies.insert(manager->sendCustomRequest(*request, "PROPFIND"),
                           aktuelleVeranstaltung);

            delete request;
        }

        // Ausführen des Requests für "Strukturierte Materialien"
        if (options->isSharedMaterialsCheckBoxChecked())
        {
            QNetworkRequest *request = webdavRequest(aktuelleVeranstaltung, "/materials/structured/");

            // Einfügen und Absenden des Requests
            replies.insert(manager->sendCustomRequest(*request, "PROPFIND"),
                           aktuelleVeranstaltung);

            delete request;

            request = webdavRequest(aktuelleVeranstaltung, "/shared/documents/");

            // Einfügen und Absenden des Requests
            replies.insert(manager->sendCustomRequest(*request, "PROPFIND"),
                           aktuelleVeranstaltung);

            delete request;
        }

        // Ausführen des Requests für "Übungsbetrieb"
        if (options->isExercisesCheckBoxChecked())
        {
            QNetworkRequest *request = webdavRequest(aktuelleVeranstaltung, "/exerciseCourse/SampleSolutions/");

            // Einfügen und Absenden des Requests
            replies.insert(manager->sendCustomRequest(*request, "PROPFIND"),
                           aktuelleVeranstaltung);

            delete request;

            request = webdavRequest(aktuelleVeranstaltung, "/exerciseCourse/AssignmentAttachments/");

            // Einfügen und Absenden des Requests
            replies.insert(manager->sendCustomRequest(*request, "PROPFIND"),
                           aktuelleVeranstaltung);

            delete request;

            request = webdavRequest(aktuelleVeranstaltung, "/exerciseCourse/AssignmentDocuments/");

            // Einfügen und Absenden des Requests
            replies.insert(manager->sendCustomRequest(*request, "PROPFIND"),
                           aktuelleVeranstaltung);

            delete request;
        }

        // Ausführen des Requests für "Literatur"
        if (options->isLiteratureCheckBoxChecked())
        {
            QNetworkRequest *request = webdavRequest(aktuelleVeranstaltung, "/literature/Lists/Literature/Attachments/");

            // Einfügen und Absenden des Requests
            replies.insert(manager->sendCustomRequest(*request, "PROPFIND"),
                           aktuelleVeranstaltung);

            delete request;
        }

        // Ausführen des Requests für "Betreuerbereich"
        if (options->isTutorDocumentsCheckBoxChecked())
        {
            QNetworkRequest *request = webdavRequest(aktuelleVeranstaltung, "/tutor/documents/");

            // Einfügen und Absenden des Requests
            replies.insert(manager->sendCustomRequest(*request, "PROPFIND"),
                           aktuelleVeranstaltung);

            delete request;
        }
    }
}

void Browser::filesRecieved(QNetworkReply *reply)
{
    // Prüfen auf Fehler
    if (!reply->error())
        Parser::parseFiles(reply, &replies,
                           options->downloadFolderLineEditText());

    // Ausgabe einer Fehlermeldung bei Fehlern
    // Ignoriere "ContentNotFoundError", der bei leeren Veranstaltungen
    // auftritt
    else if (reply->error() != QNetworkReply::ContentNotFoundError)
        Utils::errorMessageBox("Beim Abruf des Inhalts einer Veranstaltung ist ein Fehler aufgetreten", reply->errorString());;

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

        if (refreshCounter == 1)
            on_showNewDataPushButton_clicked();

        emit enableSignal(true);

        // Falls bisher noch nicht synchronisiert wurde und
        // Synchronisation beim Start aktiviert wurde
        if (options->isAutoSyncOnStartCheckBoxChecked() && refreshCounter==1 && options->getLoginCounter()==1)
            on_syncPushButton_clicked();
    }
}

void Browser::on_syncPushButton_clicked()
{
    emit enableSignal(false);
    QString directoryPath = options->downloadFolderLineEditText();

    // Falls noch kein Pfad angegeben wurde, abbrechen und FileDialog
    // öffnen
    if (directoryPath.isEmpty())
    {
        emit switchTab(1);
//        on_downloadFolderPushButton_clicked();
        emit enableSignal(true);
        return;
    }

    // Deaktivieren des DialogButtons
    //ui->downloadFolderPushButton->setEnabled(false);
    QDir verzeichnis(directoryPath);

    // Überprüfung, ob das angegebene Verzeichnis existiert, oder
    // erstellt werden kann
    if (!verzeichnis.exists() && !verzeichnis.mkpath(verzeichnis.path()))
    {
        //ui->downloadFolderPushButton->setEnabled(true);
        emit enableSignal(true);
        return;
    }

    // Hinzufügen aller eingebundenen Elemente einer Liste
    QLinkedList < Structureelement * > elementList;

    for (int i = 0; i < itemModel->rowCount(); i++)
    {
        getStructureelementsList((Structureelement *) itemModel->item(i, 0), elementList, true);
    }

    if (!elementList.isEmpty())
    {
        FileDownloader *loader = new FileDownloader(options->userNameLineEditText(),	// Benutzername
                options->userPasswordLineEditText(),	// Passwort
                getFileCount(elementList),	// Anzahl
                // der
                // zu
                // runterladenen
                // Dateien
                options->isOriginalModifiedDateCheckBoxChecked(),
                this);
        // Iterieren über alle Elemente
        Structureelement *currentDirectory = elementList.first();
        int counter = getFileCount(elementList);
        int changedCounter = 0;
        bool neueVeranstaltung = false;
        QString veranstaltungName;

        QItemSelection newSelection;
        ui->dataTreeView->collapseAll();

        for (QLinkedList < Structureelement * >::iterator iterator =
                 elementList.begin(); iterator != elementList.end(); iterator++)
        {
            Structureelement* currentElement = *iterator;
            if (currentElement->parent() != 0)
            {
                while (!currentElement->data(urlRole).toUrl().
                       toString().contains(currentDirectory->data(urlRole).
                                           toUrl().toString(),
                                           Qt::CaseSensitive))
                {
                    currentDirectory = (Structureelement *) currentDirectory->parent();
                    verzeichnis.cdUp();
                }
            }
            else
            {
                verzeichnis.setPath(options->downloadFolderLineEditText());
                neueVeranstaltung = true;
            }

            // 1. Fall: Ordner
            if (currentElement->type() != fileItem)
            {
                if (!verzeichnis.exists(currentElement->text()))
                {
                    if (!verzeichnis.mkdir(currentElement->text()))
                    {
                        Utils::errorMessageBox("Beim Erstellen eines Ordners ist ein Fehler aufgetreten.", currentElement->text());
                        break;
                    }
                }

                if (neueVeranstaltung)
                {
                    veranstaltungName = currentElement->text();
                    neueVeranstaltung = false;
                }

                currentDirectory = *iterator;
                verzeichnis.cd(currentElement->text());
            }
            // 2. Fall: Datei
            else
            {
                // Datei existiert noch nicht oder ist zu klein
                // counter++;
                if (!verzeichnis.exists(currentElement->text()) ||
                    (QFileInfo(verzeichnis, currentElement->text()).size()
                     != (*((Structureelement *) (*iterator))).data(sizeRole).toInt()))
                {
                    if (!loader->startNextDownload(currentElement->text(),
                                                   veranstaltungName,
                                                   verzeichnis.
                                                   absoluteFilePath(currentElement->text()), currentElement->data(urlRole).toUrl(), changedCounter + 1, currentElement->data(sizeRole).toInt()))
                        break;

                    changedCounter++;
                    currentElement->setData(JUST_SYNCHRONISED, synchronisedRole);
                    ui->dataTreeView->scrollTo(proxyModel.mapFromSource(currentElement->index()));
                    newSelection.select(currentElement->index(), currentElement->index());
                }
            }
        }

        loader->close();

        if (options->isAutoCloseAfterSyncCheckBoxChecked())
        {
            AutoCloseDialog autoClose;
            if(!autoClose.exec())
                QCoreApplication::quit();
        }

        // Information über abgeschlossene Synchronisation anzeigen

//        Utils::errorMessageBox("Synchronisation mit dem L2P der RWTH Aachen abgeschlossen.", QString
//                               ("Es wurden %1 von %2 eingebundenen Dateien synchronisiert.").arg
//                               (QString::number(changedCounter),
//                                QString::number(counter)));
        QMessageBox messageBox(this);
        QTimer::singleShot(10000, &messageBox, SLOT(accept()));
        messageBox.setText
        ("Synchronisation mit dem L2P der RWTH Aachen abgeschlossen.");
        messageBox.setIcon(QMessageBox::NoIcon);
        messageBox.setInformativeText(QString::fromUtf8
                                      ("Es wurden %1 von %2 eingebundenen Dateien synchronisiert.\n(Dieses Fenster schließt nach 10 Sek. automatisch.)").arg
                                      (QString::number(changedCounter),
                                       QString::number(counter)));
        messageBox.setStandardButtons(QMessageBox::Ok);
        messageBox.exec();

        // Alle synchronisierten Elemente auswählen
        ui->dataTreeView->selectionModel()->
        select(proxyModel.mapSelectionFromSource(newSelection),
               QItemSelectionModel::ClearAndSelect);
    }


    emit enableSignal(true);
}

// Diese Funktion nimmt die Eingabe im Suchfeld und zeigt alle Treffer
// an und markiert sie
void Browser::on_searchPushButton_clicked()
{
    // Kein Suchwort
    if (ui->searchLineEdit->text().isEmpty())
        return;

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
        if (topElement->child(i))
            getStructureelementsList((Structureelement*) topElement->child(i), list);
}

int Browser::getFileCount(QLinkedList < Structureelement * > &liste)
{
    // Zählen aller Dateien einer Liste
    int fileCounter = 0;

    for (QLinkedList < Structureelement * >::iterator iterator =
             liste.begin(); iterator != liste.end(); iterator++)
    {
        if ((**iterator).type() == fileItem)
            fileCounter++;
    }

    return fileCounter;
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

// Erzeugt ein webdavRequest für das L2P
// Der Benutzer muss den Request selbst wieder löschen
QNetworkRequest *Browser::webdavRequest(Structureelement *aktuelleVeranstaltung, QString urlExtension)
{
    QNetworkRequest *request = new QNetworkRequest(QUrl(aktuelleVeranstaltung->data(urlRole).toUrl().toString() % urlExtension));
    request->setRawHeader("Depth", "infinity");
    request->setRawHeader("Content-Type",
                         "text/xml; charset=\"utf-8\"");
    request->setRawHeader("Content-Length", "0");

    return request;
}

void Browser::doAuthentification(QNetworkReply *, QAuthenticator *auth)
{
    auth->setUser(options->userNameLineEditText());
    auth->setPassword(options->userPasswordLineEditText());
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
                                       (Utils::getStrukturelementPfad(element,
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
    // Öffnen der Datei auf der Festplatte des mit der rechten Maustaste
    // geklickten Items
    if (!QDesktopServices::openUrl(QUrl(Utils::getStrukturelementPfad(lastRightClickItem, options->downloadFolderLineEditText()), QUrl::TolerantMode)))
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
            item->setData(true, includeRole);
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
    Utils::copyTextToClipboard(lastRightClickItem);
}

void Browser::successfulLoginSlot()
{
    ui->refreshPushButton->setEnabled(true);
}

