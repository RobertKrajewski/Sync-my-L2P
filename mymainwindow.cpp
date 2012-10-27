/****************************************************************************
** This file is part of Sync-my-L2P.
**
** Sync-my-L2P is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Sync-my-L2P is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with Sync-my-L2P.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#include "mymainwindow.h"
#include "ui_mymainwindow.h"

// Hauptadresse des Sharepointdienstes
QString MainURL = "https://www2.elearning.rwth-aachen.de";

MyMainWindow::MyMainWindow(QWidget *parent) :
    QMainWindow(parent, Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint
                | Qt::WindowMinimizeButtonHint),
    ui(new Ui::MyMainWindow)
{
    ui->setupUi(this);

    // Hinzufügen der Daten zur Baumansicht
    itemModel = new QStandardItemModel();
    proxyModel.setDynamicSortFilter(true);
    proxyModel.setSourceModel(itemModel);
    ui->dataTreeView->setModel(&proxyModel);
    //ui->dataTreeView->setModel(&veranstaltungen);

    loadSettings();

    // Variable für das automatische Synchronisieren beim Programmstart
    autoSynchronize = false;

    // Erzeugen des NetzwerkAccessManagers
    manager = new QNetworkAccessManager(qApp);
    QObject::connect(manager, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*))
                     ,this, SLOT(doAuthentification(QNetworkReply*, QAuthenticator*)));



    // Starten der Darstellung des Fensters
    this->show();

    // Zentrieren des Fensters
    QRect desktopRect = QApplication::desktop()->screenGeometry();
    QRect windowRect  = this->frameGeometry();
    move((desktopRect.width()-windowRect.width())/2, (desktopRect.height()-windowRect.height())/2);

    // Ausführen des Autologins, falls gewünscht
    QSettings settings("Robert K.", "L2P-Tool++");
    if (settings.value("login/autoLoginOnStartCheckBox").toBool())
    {
        autoSynchronize = ui->autoSyncOnStartCheckBox->isChecked();
        on_loginPushButton_clicked();
    }
}



void MyMainWindow::loadSettings()
{
    // Laden von gespeicherten Einstellungen
    QSettings settings("Robert K.", "L2P-Tool++");

    // Laden der Logindaten
    if (settings.value("login/save").toBool())
    {
        ui->userNameLineEdit->setText(settings.value("login/benutzername","").toString());
        ui->userPasswordLineEdit->setText(settings.value("login/passwort","").toString());
        ui->userDataSaveCheckBox->setChecked(true);
        ui->autoLoginOnStartCheckBox->setEnabled(false);
    }

    // Laden des Downloadverzeichnisses
    ui->downloadFolderlineEdit->setText(settings.value("verzeichnis","").toString());
    if(!ui->downloadFolderlineEdit->text().isEmpty())
        ui->downloadFolderPushButton->setEnabled(true);

    // Laden der gesetzten Filter und sonstigen Einstellungen
    // Falls die Einstellungen nicht vorhanden sein sollten, setze Defaultwert
    ui->autoSyncOnStartCheckBox->setChecked(settings.value("autoSync").toBool());

    if (settings.contains("documents"))
        ui->documentsCheckBox->setChecked(settings.value("documents").toBool());
    else
        ui->documentsCheckBox->setChecked(true);


    if (settings.contains("structuredDocuments"))
        ui->sharedMaterialsCheckBox->setChecked(settings.value("structuredDocuments").toBool());
    else
        ui->sharedMaterialsCheckBox->setChecked(true);


    if (settings.contains("exercises"))
        ui->exercisesCheckBox->setChecked(settings.value("exercises").toBool());
    else
        ui->exercisesCheckBox->setChecked(true);


    if (settings.contains("maxSizeCB"))
    {
        ui->sizeLimitCheckBox->setChecked(settings.value("maxSizeCB").toBool());
        proxyModel.setMaximumSizeFilter(settings.value("maxSizeCB").toBool());
    }


    if (settings.contains("maxSizeB"))
    {
        ui->sizeLimitSpinBox->setValue(settings.value("maxSizeB").toInt());
        proxyModel.setMaximumSize(settings.value("maxSizeB").toInt());
    }
    else
        ui->sizeLimitSpinBox->setValue(10);
}

MyMainWindow::~MyMainWindow()
{
    saveSettings();
    delete ui;
}

void MyMainWindow::saveSettings()
{
    // Speichern aller Einstellungen
    QSettings settings("Robert K.", "L2P-Tool++");
    settings.setValue("login/save", ui->userDataSaveCheckBox->isChecked());
    if (ui->userDataSaveCheckBox->isChecked())
    {
        settings.setValue("login/benutzername", ui->userNameLineEdit->text());
        settings.setValue("login/passwort", ui->userPasswordLineEdit->text());
        settings.setValue("login/autoLoginOnStartCheckBox", ui->autoLoginOnStartCheckBox->isChecked());
    }
    else
    {
        settings.remove("login");
    }
    settings.setValue("verzeichnis", ui->downloadFolderlineEdit->text());
    settings.setValue("autoSync", ui->autoSyncOnStartCheckBox->isChecked());
    settings.setValue("documents", ui->documentsCheckBox->isChecked());
    settings.setValue("structuredDocuments", ui->sharedMaterialsCheckBox->isChecked());
    settings.setValue("exercises", ui->exercisesCheckBox->isChecked());
    settings.setValue("maxSizeCB", ui->sizeLimitCheckBox->isChecked());
    settings.setValue("maxSizeB", ui->sizeLimitSpinBox->value());
}

void MyMainWindow::on_refreshPushButton_clicked()
{
    // Löschen der alten Veranstaltungsliste
    itemModel->clear();

    // Zurücksetzen der freigeschalteten Schaltflächen
    ui->refreshPushButton->             setEnabled(false);
    ui->removeSelectionPushButton->     setEnabled(false);
    ui->addSelectionPushButton->        setEnabled(false);
    ui->syncPushButton->                setEnabled(false);

    // Einfrieren der Anzeige
    ui->centralwidget->                 setEnabled(false);

    // Verbinden des Managers
    QObject::connect(manager, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(veranstaltungenAbgerufen(QNetworkReply*)));

    // Starten der Anfrage
    if (ui->currentSemesterCheckBox->isChecked())
        replies.insert(manager->get(QNetworkRequest(QUrl(MainURL % "/foyer/summary/default.aspx"))), 0);

    // Starten einer zweiten Anfrage für ältere Semester, falls eingestellt
    if(ui->oldSemesterCheckBox->isChecked())
        replies.insert(manager->get(QNetworkRequest(QUrl(MainURL % "/foyer/archive/default.aspx"))), 0);
}

void MyMainWindow::veranstaltungenAbgerufen(QNetworkReply* reply)
{
    // Prüfen auf Fehler beim Abruf
    if(!reply->error())
    {
        Parser::parseCourses(reply, itemModel);
    }
    else
    {
        // Falls ein Fehler aufgetreten sein sollte, Ausgabe dessen
        QMessageBox messageBox;
        messageBox.setText("Beim Abruf der Veranstaltungen ist ein Fehler aufgetreten");
        messageBox.setInformativeText(reply->errorString());
        messageBox.setStandardButtons(QMessageBox::Ok);
        messageBox.exec();
        autoSynchronize = false;
    }

    // Löschen der Antwort aus der Queue
    replies.remove(reply);

    // Löschen der Antwort aus dem Speicher
    reply->deleteLater();

    // Prüfen, ob noch Antworten ausstehen und ggf. Reaktiveren der Benutzeroberfläche
    if (replies.isEmpty())
    {
        // Veranstaltungen alphabetisch sortieren
        itemModel->sort(0);

        QObject::disconnect(manager, SIGNAL(finished(QNetworkReply*)),
                            this, SLOT(veranstaltungenAbgerufen(QNetworkReply*)));

        // Aufruf der Funktion zur Aktualisierung der Dateien
        dateienAktualisieren();
    }
}

void MyMainWindow::dateienAktualisieren()
{
    // Prüfen, ob überhaupt Dokumentorte ausgewählt wurden
    if (!ui->documentsCheckBox->isChecked() && !ui->sharedMaterialsCheckBox->isChecked() && !ui->exercisesCheckBox->isChecked())
    {
        // Freischalten von Schaltflächen
        ui->refreshPushButton->setEnabled(true);
        ui->centralwidget->setEnabled(true);
        autoSynchronize = false;
        return;
    }

    QObject::connect(manager, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(dateienAbgerufen(QNetworkReply*)));

    // Durchlaufen aller Veranstaltungen
    int rowCount = itemModel->rowCount();
    Structureelement* aktuelleStruktur = 0;

    // Übungsbetrieb Lösungen
    // "https://www2.elearning.rwth-aachen.de/ws10/10ws-02568/exerciseCourse/SampleSolutions/"

    // Übungsbetrieb Blätter
    // "https://www2.elearning.rwth-aachen.de/ss11/11ss-33668/exerciseCourse/AssignmentDocuments/

    for(int i= 0; i < rowCount; i++)
    {
        // Holen der aktuellen Veranstaltung
        aktuelleStruktur = (Structureelement*) itemModel->item(i);

        // Löschen aller Dateien
        if(aktuelleStruktur->rowCount() > 0)
            aktuelleStruktur->removeRows(0, aktuelleStruktur->rowCount());

        // Ausführen des Requests für "Dokumente"
        if (ui->documentsCheckBox->isChecked())
        {
            // Erstellen eines WebDav Requests
            QNetworkRequest request(QUrl(aktuelleStruktur->data(urlRole).toUrl().toString() % "/materials/documents/"));
            request.setRawHeader("Depth", "infinity");
            request.setRawHeader("Content-Type", "text/xml; charset=\"utf-8\"");
            request.setRawHeader("Content-Length", "0");
            QList<QByteArray> HeaderList = request.rawHeaderList();
            foreach(QByteArray Header, HeaderList)
            {
               //qDebug(Header);
            }

            // Einfügen und Absenden des Requests
            replies.insert(manager->sendCustomRequest(request, "PROPFIND"), aktuelleStruktur);
        }

        // Ausführen des Requests für "Strukturierte Materialien"
        if (ui->sharedMaterialsCheckBox->isChecked())
        {
            // Erstellen eines WebDav Requests
            QNetworkRequest request2(QUrl(aktuelleStruktur->data(urlRole).toUrl().toString() % "/materials/structured/"));
            request2.setRawHeader("Depth", "infinity");
            request2.setRawHeader("Content-Type", "text/xml; charset=\"utf-8\"");
            request2.setRawHeader("Content-Length", "0");

            // Einfügen in die Map und Absenden des Requests
            replies.insert(manager->sendCustomRequest(request2, "PROPFIND"), aktuelleStruktur);
        }

        if (ui->exercisesCheckBox->isChecked())
        {
            // Erstellen eines WebDav Requests
            QNetworkRequest request(QUrl(aktuelleStruktur->data(urlRole).toUrl().toString() % "/exerciseCourse/SampleSolutions/"));
            request.setRawHeader("Depth", "infinity");
            request.setRawHeader("Content-Type", "text/xml; charset=\"utf-8\"");
            request.setRawHeader("Content-Length", "0");

            // Einfügen und Absenden des Requests
            replies.insert(manager->sendCustomRequest(request, "PROPFIND"), aktuelleStruktur);

            // Erstellen eines WebDav Requests
            QNetworkRequest request2(QUrl(aktuelleStruktur->data(urlRole).toUrl().toString() % "/exerciseCourse/AssignmentDocuments/"));
            request2.setRawHeader("Depth", "infinity");
            request2.setRawHeader("Content-Type", "text/xml; charset=\"utf-8\"");
            request2.setRawHeader("Content-Length", "0");

            // Einfügen und Absenden des Requests
            replies.insert(manager->sendCustomRequest(request2, "PROPFIND"), aktuelleStruktur);
        }
    }

    // Sonderfall: Es existieren keine Veranstaltungen
    if (rowCount == 0)
    {
        QObject::disconnect(manager, SIGNAL(finished(QNetworkReply*)),
                            this, SLOT(dateienAbgerufen(QNetworkReply*)));

        // Freischalten von Schaltflächen
        ui->refreshPushButton-> setEnabled(true);
        ui->addSelectionPushButton->     setEnabled(true);
        ui->removeSelectionPushButton-> setEnabled(true);
        ui->syncPushButton->setEnabled(true);
        ui->centralwidget-> setEnabled(true);

        autoSynchronize = false;
    }
}

void MyMainWindow::on_searchPushButton_clicked()
{
    // Kein Suchwort
    if (ui->searchLineEdit->text().isEmpty())
        return;


    QStandardItem *item;
    QList<QStandardItem *> found = itemModel->findItems(
                "*"%ui->searchLineEdit->text()%"*", Qt::MatchWildcard | Qt::MatchRecursive);
    QItemSelection newSelection;
    ui->dataTreeView->collapseAll();
    foreach (item, found) {
        newSelection.select(item->index(),item->index());
        ui->dataTreeView->scrollTo(proxyModel.mapFromSource(item->index()));
    }
    ui->dataTreeView->selectionModel()->select(proxyModel.mapSelectionFromSource(newSelection), QItemSelectionModel::ClearAndSelect);
}

void MyMainWindow::dateienAbgerufen(QNetworkReply* reply)
{
    // Prüfen auf Fehler
    if (!reply->error())
    {
        Parser::parseFiles(reply, &replies, ui->downloadFolderlineEdit->text());
    }
    // Ausgabe einer Fehlermeldung bei Fehlern
    // Ignoriere "ContentNotFoundError", der bei leeren Veranstaltungen auftritt
    else if(reply->error() != QNetworkReply::ContentNotFoundError)
    {
        QMessageBox messageBox;
        messageBox.setText("Beim Abruf des Inhalts einer Veranstaltung ist ein Fehler aufgetreten");
        messageBox.setInformativeText(reply->errorString());
        messageBox.setStandardButtons(QMessageBox::Ok);
        messageBox.exec();
    }

    // Entfernen von leeren Veranstaltungen
//    Strukturelement* aktuelleVeranstaltung = replies.value(reply);
//    if(!aktuelleVeranstaltung->hasChildren())
//    {
//        veranstaltungen.removeRow(veranstaltungen.indexFromItem(aktuelleVeranstaltung).row());
//    }

    // Löschen der Antwort aus der Liste der abzuarbeitenden Antworten
    replies.remove(reply);

    // Freigabe des Speichers
    reply->deleteLater();

    // Prüfen, ob alle Antworten bearbeitet wurden -> Replies.empty() = TRUE
    if(replies.empty())
    {
        QObject::disconnect(manager, SIGNAL(finished(QNetworkReply*)),
                            this, SLOT(dateienAbgerufen(QNetworkReply*)));

        // Freischalten von Schaltflächen
        ui->refreshPushButton-> setEnabled(true);
        ui->addSelectionPushButton->     setEnabled(true);
        ui->removeSelectionPushButton-> setEnabled(true);
        ui->syncPushButton->setEnabled(true);
        ui->centralwidget-> setEnabled(true);

        // Falls bisher noch nicht synchronisiert wurde und Synchronisation beim Start aktiviert wurde
        if (autoSynchronize)
        {
            autoSynchronize = false;
            on_syncPushButton_clicked();
        }

    }
}

void MyMainWindow::on_removeSelectionPushButton_clicked()
{
    // Holen der ausgewählten Dateien
    QModelIndexList ausgewaehlt = proxyModel.mapSelectionToSource(ui->dataTreeView->selectionModel()->selection()).indexes();

    // Iteration über alle Elemente
    Structureelement* aktuelleStruktur = 0;
    QModelIndexList::Iterator iteratorEnd = ausgewaehlt.end();
    for(QModelIndexList::Iterator iter = ausgewaehlt.begin(); iter != iteratorEnd; iter++)
    {
        // Holen des Vaterelements des ausgewählten Strukturelements
        aktuelleStruktur = (Structureelement*) iter->internalPointer();

        // Ausschließen des ausgewählten Elements
        Structureelement* temp = (Structureelement*)aktuelleStruktur->child(iter->row(),0);
        ausschliessen(temp);
        //ausschliessen((Strukturelement*)aktuelleStruktur->child(iter->row(),0));

        // Prüfen, ob alle Geschwisterelemente ausgeschlossen sind
        // => Ausschließen des Vaterlements
        if(aktuelleStruktur != itemModel->invisibleRootItem())
        {
            bool siblingsExcluded = true;

            // Prüfung aller Zeilen, ob alle ausgeschlossen
            for (int i=0; i < aktuelleStruktur->rowCount(); i++)
            {
                if(((Structureelement*)aktuelleStruktur->child(i,0))->data(includeRole).toBool())
                    siblingsExcluded = false;
            }

            // Falls ja, Vaterelement auch ausschließen
            if (siblingsExcluded)
            {
                aktuelleStruktur->setData(false, includeRole);
                //aktuelleStruktur = (Strukturelement*) aktuelleStruktur->parent();
            }
            else
            {
            }
        }
    }

    // Aktualisieren der Ansicht
    ui->dataTreeView->dataChanged(itemModel->index(0,0),itemModel->index(itemModel->rowCount(),0));
}

void MyMainWindow::ausschliessen(Structureelement* aktuelleStruktur)
{
    // Ausschließen aller untergeordneten Elemente
    if(aktuelleStruktur->data(includeRole).toBool()){
        aktuelleStruktur->setData(false, includeRole);
        for(int i = 0; i < aktuelleStruktur->rowCount(); i++)
        {
            ausschliessen((Structureelement*)aktuelleStruktur->child(i,0));
        }
    }
}

void MyMainWindow::on_addSelectionPushButton_clicked()
{
    // Bestimmen der ausgewählten Items
    QModelIndexList selectedItemsList =  proxyModel.mapSelectionToSource(ui->dataTreeView->selectionModel()->selection()).indexes();

    // Variablen zur Speicherung von Pointern aus Performancegründen vor der Schleife
    Structureelement* aktuelleStruktur = 0;
    QModelIndexList::Iterator iteratorEnd = selectedItemsList.end();

    for(QModelIndexList::Iterator iterator = selectedItemsList.begin(); iterator != iteratorEnd; iterator++)
    {
        // Holen des Pointers auf das ausgewählte Item
        aktuelleStruktur = (Structureelement*)((Structureelement*) iterator->internalPointer())->child(iterator->row(),0);

        // Einbinden aller übergeordneter Ordner
        Structureelement* parent = aktuelleStruktur;
        while((parent = (Structureelement*) parent->parent()) != 0)
        {
            if (parent->data(includeRole).toBool())
                break;
            parent->setData(true, includeRole);
        }

        // Einbinden aller untergeordneter Ordner und Dateien
        einbinden(aktuelleStruktur);
    }

    // Aktualisierung der Ansicht
    ui->dataTreeView->dataChanged(itemModel->index(0,0),itemModel->index(itemModel->rowCount(),0));
}

void MyMainWindow::einbinden(Structureelement* aktuelleStruktur)
{
    // Einbinden des aktuellen Items
    aktuelleStruktur->setData(true, includeRole);

    // Einbinden aller untergeordnete Ordner und Dateien
    int rowCount = aktuelleStruktur->rowCount();
    for(int i = 0; i < rowCount; i++)
    {
        einbinden((Structureelement*)aktuelleStruktur->child(i,0));
    }
}

void MyMainWindow::on_loginPushButton_clicked()
{
    // Erstellen eines Logintesters
    LoginTester* LoginTest = new LoginTester(ui->userNameLineEdit->text(), ui->userPasswordLineEdit->text(), 2, this);

    // Testen des Logins
    // 1.Fall: Login erfolgreich
    if(LoginTest->exec())
    {
        // Ã„ndern des Schreibrechts der LineEdits
        ui->userNameLineEdit->setReadOnly(true);
        ui->userPasswordLineEdit->setReadOnly(true);

        // Hinzufügen eines neuen Styles
        ui->userNameLineEdit->setStyleSheet("QLineEdit{background-color:#6CFF47; font: bold}");
        ui->userPasswordLineEdit->setStyleSheet("QLineEdit{background-color:#6CFF47; font: bold}");

        // Aktualisieren der Buttons
        ui->loginPushButton->setEnabled(false);
        ui->refreshPushButton->setEnabled(true);
        ui->autoLoginOnStartCheckBox->setEnabled(true);
        ui->userDataSaveCheckBox->setEnabled(true);

        // Setzen des buttons je nach Einstellung
        QSettings einstellungen("Robert K.", "L2P-Tool++");
//        ui->userDataSaveCheckBox->setChecked(einstellungen.value("login/save").toBool());
        ui->autoLoginOnStartCheckBox->setChecked(einstellungen.value("login/autoLoginOnStartCheckBox").toBool());
        ui->autoSyncOnStartCheckBox->setChecked(einstellungen.value("autoSync").toBool());

        // Anzeigen des Erfolgs in der Statusbar
        ui->statusBar->showMessage("Login erfolgreich!");

        // Sofortiges Aktualisiern der Daten
        on_refreshPushButton_clicked();
    }
    // 2.Fall: Login fehlgeschlagen
    else
    {
        autoSynchronize = false;
    }
    delete LoginTest;
}

void MyMainWindow::on_userDataSaveCheckBox_stateChanged(int checked)
{
    // (De-)Aktivierung der autoLoginOnStartCheckBox CB
    if(checked)
        ui->autoLoginOnStartCheckBox->setEnabled(true);
    else
        ui->autoLoginOnStartCheckBox->setEnabled(false);

    // Löschen des Hakens
    ui->autoLoginOnStartCheckBox->setChecked(false);
}

void MyMainWindow::on_autoLoginOnStartCheckBox_stateChanged(int checked)
{
    // (De-)Aktivierung der autoLoginOnStartCheckBox CB
    if(checked)
        ui->autoSyncOnStartCheckBox->setEnabled(true);
    else
        ui->autoSyncOnStartCheckBox->setEnabled(false);

    // Löschen des Hakens
    ui->autoSyncOnStartCheckBox->setChecked(false);
}

void MyMainWindow::on_userNameLineEdit_textChanged(const QString)
{
    aktiviereLoginButton();
}

void MyMainWindow::on_userPasswordLineEdit_textChanged(const QString)
{
    aktiviereLoginButton();
}

void MyMainWindow::aktiviereLoginButton()
{
    // Löschen der gespeicherten Einstellungen, da neue Benutzerdaten vorliegen
    ui->autoLoginOnStartCheckBox->setChecked(false);
    ui->userDataSaveCheckBox->setChecked(false);


    // Aktivieren des Loginbuttons, wenn beide Felder ausgefüllt sind
    if(!ui->userNameLineEdit->text().isEmpty() && !ui->userPasswordLineEdit->text().isEmpty())
    {
        ui->loginPushButton->setEnabled(true);
    }
    else
    {
        ui->loginPushButton->setEnabled(false);
    }
}

void MyMainWindow::on_syncPushButton_clicked()
{
    ui->centralwidget->setEnabled(false);
    QString directoryPath = ui->downloadFolderlineEdit->text();

    // Falls noch kein Pfad angegeben wurde, abbrechen und FileDialog öffnen
    if (directoryPath.isEmpty())
    {
        ui->tabWidget->setCurrentIndex(1);
        on_downloadFolderPushButton_clicked();
        ui->centralwidget->setEnabled(true);
        return;
    }
    // Deaktivieren des DialogButtons
    ui->downloadFolderPushButton->setEnabled(false);

    QDir verzeichnis(directoryPath);

    // Überprüfung, ob das angegebene Verzeichnis existiert, oder erstellt werden kann
    if(!verzeichnis.exists() && !verzeichnis.mkpath(verzeichnis.path()))
    {
        ui->downloadFolderPushButton->setEnabled(true);
        ui->centralwidget->setEnabled(true);
        return;   
    }

    // Hinzufügen aller eingebundenen Elemente einer Liste
    QLinkedList<Structureelement*> liste;
    for(int i=0; i < itemModel->rowCount(); i++)
    {
        getStrukturelementeListe((Structureelement*)itemModel->item(i,0), liste, true);
    }

    if(!liste.isEmpty()){

        FileDownloader* loader = new FileDownloader(ui->userNameLineEdit->text(), // Benutzername
                                                      ui->userPasswordLineEdit->text(),     // Passwort
                                                      getFileCount(liste),          // Anzahl der zu runterladenen Dateien
                                                      this);

        // Iterieren über alle Elemente
        Structureelement* aktuellerOrdner = liste.first();
        int counter = getFileCount(liste);
        int changedCounter = 0;
        bool neueVeranstaltung = false;
        QString veranstaltungName;
        QList<QString> downloadedItems;
        for(QLinkedList<Structureelement*>::iterator iterator = liste.begin();
            iterator != liste.end();
            iterator++)
        {
            if((**iterator).parent() != 0)
            {
                while(!(**iterator).data(urlRole).toUrl().toString().contains(
                          aktuellerOrdner->data(urlRole).toUrl().toString(), Qt::CaseSensitive))
                {

                    aktuellerOrdner = (Structureelement*) aktuellerOrdner->parent();
                    verzeichnis.cdUp();
                }
            }
            else
            {
                verzeichnis.setPath(ui->downloadFolderlineEdit->text());
                neueVeranstaltung = true;
            }
            // 1. Fall: Ordner
            if((**iterator).type() != fileItem)
            {
                if(!verzeichnis.exists((**iterator).text()))
                {
                    if (!verzeichnis.mkdir((**iterator).text()))
                    {
                        QMessageBox messageBox;
                        messageBox.setText("Beim Erstellen eines Ordners ist ein Fehler aufgetreten.");
                        messageBox.setInformativeText((**iterator).text());
                        messageBox.setStandardButtons(QMessageBox::Ok);
                        messageBox.exec();
                        break;
                    }
                }
                if (neueVeranstaltung)
                {
                    veranstaltungName = (**iterator).text();
                    neueVeranstaltung = false;
                }

                aktuellerOrdner = *iterator;
                verzeichnis.cd((**iterator).text());
            }
            // 2. Fall: Datei
            else
            {
                // Datei existiert noch nicht
                //counter++;
                if (!verzeichnis.exists((**iterator).text()) ||
                        (QFileInfo(verzeichnis, (**iterator).text()).size()
                         != (*((Structureelement*)(*iterator))).data(sizeRole).toInt()))
                {
                    if (!loader->startNextDownload((**iterator).text(),
                                                  veranstaltungName,
                                                  verzeichnis.absoluteFilePath((**iterator).text()),
                                                  (**iterator).data(urlRole).toUrl(),
                                                  changedCounter+1,
                                                  (**iterator).data(sizeRole).toInt()))
                        break;
                    changedCounter++;
                    (**iterator).setData(NOW_SYNCHRONISED, synchronisedRole);
                    downloadedItems.append((**iterator).text());
                }
            }
        }
        loader->close();

        QString downloadedItemsString;

        QString nstring;
        foreach(nstring, downloadedItems)
        {
            downloadedItemsString.append(nstring%"\n");
        }

        QMessageBox messageBox(this);
        messageBox.setText("Synchronisation mit dem L2P der RWTH Aachen abgeschlossen.");
        messageBox.setIcon(QMessageBox::NoIcon);
        messageBox.setInformativeText(QString("Es wurden %1 von %2 eingebundenen Dateien synchronisiert.")
                                      .arg(QString::number(changedCounter), QString::number(counter)));
        messageBox.setDetailedText(downloadedItemsString);
        messageBox.setStandardButtons(QMessageBox::Ok);
        messageBox.exec();
    }
    ui->downloadFolderPushButton->setEnabled(true);
    ui->centralwidget->setEnabled(true);
}

void MyMainWindow::getStrukturelementeListe(Structureelement* aktuellesElement, QLinkedList<Structureelement*>& liste, bool onlyIncluded)
{
    if(!onlyIncluded || (onlyIncluded && aktuellesElement->data(includeRole).toBool()))
    {
        if ((ui->sizeLimitCheckBox->isChecked() && aktuellesElement->data(sizeRole).toInt() < (ui->sizeLimitSpinBox->value() * 1024 * 1024))
                || !ui->sizeLimitCheckBox->isChecked())
        liste.append(aktuellesElement);
        if(aktuellesElement->hasChildren())
            for (int i = 0; i < aktuellesElement->rowCount(); i++)
                getStrukturelementeListe((Structureelement*)aktuellesElement->child(i, 0), liste, onlyIncluded);
    }
}

int MyMainWindow::getFileCount(QLinkedList<Structureelement*>& liste)
{
    // Zählen aller Dateien einer Liste
    int fileCounter = 0;
    for(QLinkedList<Structureelement*>::iterator iterator = liste.begin(); iterator != liste.end(); iterator++)
    {
        if ((**iterator).type() == fileItem)
            fileCounter++;
    }
    return fileCounter;
}

void MyMainWindow::doAuthentification(QNetworkReply*, QAuthenticator* auth)
{
    auth->setUser(ui->userNameLineEdit->text());
    auth->setPassword(ui->userPasswordLineEdit->text());
}

void MyMainWindow::unknownError()
{

}

void MyMainWindow::on_downloadFolderPushButton_clicked()
{
    // Aufruf des Ordnerdialogs
    QString newDirectory = QFileDialog::getExistingDirectory(this,
                                                             "Downloadverkzeichnis auswählen",
                                                             QDir::rootPath(),
                                                             QFileDialog::ShowDirsOnly| QFileDialog::DontResolveSymlinks);
    if (!newDirectory.isEmpty())
        ui->downloadFolderlineEdit->setText(newDirectory);

    // Aktualisierung des Zustands des Buttons "Speicherort öffnen"
    if(!ui->downloadFolderlineEdit->text().isEmpty())
        ui->openDownloadfolderPushButton->setEnabled(true);
    else
        ui->openDownloadfolderPushButton->setEnabled(false);
}


void MyMainWindow::on_openDownloadfolderPushButton_clicked()
{
    // Betriebssystem soll mit dem Standardprogramm den Pfad öffnen
    QDesktopServices::openUrl(QUrl("file:///" % ui->downloadFolderlineEdit->text(), QUrl::TolerantMode));
}

void MyMainWindow::on_expandPushButton_clicked()
{
    // Expandieren aller Zweige
    ui->dataTreeView->expandAll();
}

void MyMainWindow::on_contractPushButton_clicked()
{
    // Reduktion aller Zweige
    ui->dataTreeView->collapseAll();
}

void MyMainWindow::on_dataTreeView_doubleClicked(const QModelIndex &index)
{
    Structureelement* element = (Structureelement*) itemModel->itemFromIndex(proxyModel.mapToSource(index));
    if(element->type() == fileItem)
        if(!QDesktopServices::openUrl(QUrl(Utils::getStrukturelementPfad(element, ui->downloadFolderlineEdit->text()), QUrl::TolerantMode)))
        {
            QDesktopServices::openUrl(element->data(urlRole).toUrl());
        }
}


void MyMainWindow::on_dataTreeView_customContextMenuRequested(const QPoint &pos)
{
    // Bestimmung des Elements, auf das geklickt wurde
    Structureelement* RightClickedItem = (Structureelement*) itemModel->itemFromIndex(proxyModel.mapToSource(ui->dataTreeView->indexAt(pos)));

    // Überprüfung, ob überhaupt auf ein Element geklickt wurde (oder ins Leere)
    if (RightClickedItem == 0)
        return;

    // Speichern des geklickten Items
    lastRightClickItem = RightClickedItem;


    // Erstellen eines neuen Menus
    QMenu newCustomContextMenu(this);

    // Öffnen der Veranstaltungsseite im L2P
    if (RightClickedItem->type() == courseItem)
    {
        QAction* openCourseAction = new QAction("Veranstaltungsseite öffnen", this);
        newCustomContextMenu.addAction(openCourseAction);
        QObject::connect(openCourseAction, SIGNAL(triggered()), this, SLOT(openCourse()));
    }

    // Öffnen der Datei
    QAction* openAction = new QAction("Öffnen", this);
    newCustomContextMenu.addAction(openAction);
    QObject::connect(openAction, SIGNAL(triggered()), this, SLOT(openItem()));

    // Kopieren der URL
    QAction* copyAction = new QAction("Link kopieren", this);
    newCustomContextMenu.addAction(copyAction);
    QObject::connect(copyAction, SIGNAL(triggered()), this, SLOT(copyUrlToClipboardSlot()));

    // Anzeigen des Menus an der Mausposition
    newCustomContextMenu.exec(ui->dataTreeView->mapToGlobal(pos));
}

void MyMainWindow::openCourse()
{
    // Öffnen der URL des mit der rechten Maustaste geklickten Items
    QDesktopServices::openUrl(lastRightClickItem->data(urlRole).toUrl());
}

void MyMainWindow::openItem()
{
    // Öffnen der Datei auf der Festplatte des mit der rechten Maustaste geklickten Items
    if(!QDesktopServices::openUrl(QUrl(Utils::getStrukturelementPfad(lastRightClickItem, ui->downloadFolderlineEdit->text()), QUrl::TolerantMode)))
    {
        // Öffnen der Datei im L2P des mit der rechten Maustaste geklickten Items
        QDesktopServices::openUrl(lastRightClickItem->data(urlRole).toUrl());
    }
}

void MyMainWindow::on_sizeLimitSpinBox_valueChanged(int newMaximumSize)
{
    proxyModel.setMaximumSize(newMaximumSize);
}

void MyMainWindow::on_sizeLimitCheckBox_toggled(bool checked)
{
    proxyModel.setMaximumSizeFilter(checked);
}

void MyMainWindow::on_dateFilterCheckBox_toggled(bool checked)
{
    proxyModel.setInRangeDateFilter(checked);
}

void MyMainWindow::on_minDateEdit_dateChanged(const QDate &date)
{
    proxyModel.setFilterMinimumDate(date);
}

void MyMainWindow::on_maxDateEdit_dateChanged(const QDate &date)
{
    proxyModel.setFilterMaximumDate(date);
}

void MyMainWindow::copyUrlToClipboardSlot()
{
    Utils::copyTextToClipboard(lastRightClickItem);
}
