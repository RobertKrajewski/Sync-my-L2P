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

#include "hauptfenster.h"
#include "ui_hauptfenster.h"

// Hauptadresse des Sharepointdienstes
QString StammURL = "https://www2.elearning.rwth-aachen.de";

Hauptfenster::Hauptfenster(QWidget *parent) :
    QMainWindow(parent, Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint
                | Qt::WindowMinimizeButtonHint),
    ui(new Ui::Hauptfenster)
{
    ui->setupUi(this);

    // Laden von gespeicherten Einstellungen
    QSettings einstellungen("Robert K.", "L2P-Tool++");

    // Hinzufügen der Daten zur Baumansicht
    proxyModel.setDynamicSortFilter(true);
    proxyModel.setSourceModel(&itemModel);
    ui->treeView->setModel(&proxyModel);
    //ui->treeView->setModel(&veranstaltungen);

    // Laden der Logindaten
    if (einstellungen.value("login/save").toBool())
    {
        ui->BenutzernameFeld->setText(einstellungen.value("login/benutzername","").toString());
        ui->PasswortFeld->setText(einstellungen.value("login/passwort","").toString());
        ui->DatenSpeichern->setChecked(true);
        ui->AutoLogin->setEnabled(false);
    }

    // Laden des Downloadverzeichnisses
    ui->VerzeichnisFeld->setText(einstellungen.value("verzeichnis","").toString());
    if(!ui->VerzeichnisFeld->text().isEmpty())
        ui->directoryOpen->setEnabled(true);

    // Laden der gesetzten Filter und sonstigen Einstellungen
    // Falls die Einstellungen nicht vorhanden sein sollten, setze Defaultwert
    ui->autoSyncCheck->setChecked(einstellungen.value("autoSync").toBool());

    if (einstellungen.contains("documents"))
        ui->documentsCheck->setChecked(einstellungen.value("documents").toBool());
    else
        ui->documentsCheck->setChecked(true);


    if (einstellungen.contains("structuredDocuments"))
        ui->structeredDocumentsCheck->setChecked(einstellungen.value("structuredDocuments").toBool());
    else
        ui->structeredDocumentsCheck->setChecked(true);


    if (einstellungen.contains("exercises"))
        ui->exercisesCheck->setChecked(einstellungen.value("exercises").toBool());
    else
        ui->exercisesCheck->setChecked(true);


    if (einstellungen.contains("maxSizeCB"))
    {
        ui->maxSizeCheckBox->setChecked(einstellungen.value("maxSizeCB").toBool());
        proxyModel.setMaximumSizeFilter(einstellungen.value("maxSizeCB").toBool());
    }


    if (einstellungen.contains("maxSizeB"))
    {
        ui->maxSizeBox->setValue(einstellungen.value("maxSizeB").toInt());
        proxyModel.setMaximumSize(einstellungen.value("maxSizeB").toInt());
    }
    else
        ui->maxSizeBox->setValue(10);

    // Variable für das automatische Synchronisieren beim Programmstart
    autoSynchronize = false;

    // Erzeugen des NetzwerkAccessManagers
    manager = new QNetworkAccessManager(qApp);
    QObject::connect(manager, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*))
                     ,this, SLOT(authentifizieren(QNetworkReply*, QAuthenticator*)));



    // Starten der Darstellung des Fensters
    this->show();

    // Zentrieren des Fensters
    QRect desktopRect = QApplication::desktop()->screenGeometry();
    QRect windowRect  = this->frameGeometry();
    move((desktopRect.width()-windowRect.width())/2, (desktopRect.height()-windowRect.height())/2);

    // Ausführen des Autologins, falls gewünscht
    if (einstellungen.value("login/autoLogin").toBool())
    {
        autoSynchronize = ui->autoSyncCheck->isChecked();
        on_Login_clicked();
    }
}

Hauptfenster::~Hauptfenster()
{
    // Speichern aller Einstellungen
    QSettings einstellungen("Robert K.", "L2P-Tool++");
    einstellungen.setValue("login/save", ui->DatenSpeichern->isChecked());
    if (ui->DatenSpeichern->isChecked())
    {
        einstellungen.setValue("login/benutzername", ui->BenutzernameFeld->text());
        einstellungen.setValue("login/passwort", ui->PasswortFeld->text());
        einstellungen.setValue("login/autoLogin", ui->AutoLogin->isChecked());
    }
    else
    {
        einstellungen.remove("login");
    }
    einstellungen.setValue("verzeichnis", ui->VerzeichnisFeld->text());
    einstellungen.setValue("autoSync", ui->autoSyncCheck->isChecked());
    einstellungen.setValue("documents", ui->documentsCheck->isChecked());
    einstellungen.setValue("structuredDocuments", ui->structeredDocumentsCheck->isChecked());
    einstellungen.setValue("exercises", ui->exercisesCheck->isChecked());
    einstellungen.setValue("maxSizeCB", ui->maxSizeCheckBox->isChecked());
    einstellungen.setValue("maxSizeB", ui->maxSizeBox->value());
    delete ui;
}

void Hauptfenster::on_Aktualisieren_clicked()
{
    // Löschen der alten Veranstaltungsliste
    itemModel.clear();

    // Zurücksetzen der freigeschalteten Schaltflächen
    ui->Aktualisieren->     setEnabled(false);
    ui->ausschliessen->     setEnabled(false);
    ui->einbinden->         setEnabled(false);
    ui->synchronisieren->   setEnabled(false);

    // Einfrieren der Anzeige
    ui->centralwidget->     setEnabled(false);

    // Verbinden des Managers
    QObject::connect(manager, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(veranstaltungenAbgerufen(QNetworkReply*)));

    // Starten der Anfrage
    QNetworkReply* reply = manager->get(QNetworkRequest(QUrl(StammURL % "/foyer/summary/default.aspx")));
    replies.insert(reply, 0);

    // Starten einer zweiten Anfrage für ältere Semester, falls eingestellt
    if(ui->alteSemesterCheck->isChecked())
        replies.insert(manager->get(QNetworkRequest(QUrl(StammURL % "/foyer/archive/default.aspx"))), 0);
}

void Hauptfenster::veranstaltungenAbgerufen(QNetworkReply* reply)
{

    // Prüfen auf Fehler beim Abruf
    if(!reply->error())
    {
        // Auslesen der kompletten Antwort
        QString replyText = reply->readAll();
        QMessageBox messageBox;
        messageBox.setText("Inhalt");
        messageBox.setDetailedText(replyText);
        messageBox.setStandardButtons(QMessageBox::Ok);
        messageBox.exec();

        // Erstellen eines RegExps für das Herausfiltern der Veranstaltungen
        QString regPattern = "<td class=\"ms-vb2\"><a href=\"(/(?:ws|ss)\\d{2}/\\d{2}(?:ws|ss)-\\d{5})/information/default.aspx\">(.{,230})</a></td><td";
        QRegExp* regExp = new QRegExp(regPattern, Qt::CaseSensitive);

        // Erstellen eines RegExps  für unzulässige Buchstaben im Veranstaltungsnamen
        QString escapePattern = "(:|<|>|/|\\\\|\\||\\*|\\^|\\?|\\\")";
        QRegExp* escapeRegExp = new QRegExp(escapePattern, Qt::CaseSensitive);

        // Speichern der Suchpositionen in der Antwort
        int altePosition = 0;
        int neuePosition = 0;

        // neue Veranstaltung sowie Daten
        Strukturelement* neueVeranstaltung;
        QString urlRaum;
        QString veranstaltungName;

        // Durchsuchen der gesamten Antwort nach Veranstaltungen
        while((neuePosition=regExp->indexIn(replyText, altePosition)) != -1)
        {
            // Anpassen der Encodierung wegen der Umlaute
            urlRaum = QString::fromUtf8(regExp->cap(1).toLatin1());
            veranstaltungName = QString::fromUtf8(regExp->cap(2).toLatin1());
            veranstaltungName.replace(*escapeRegExp, "");


            // Erstellen der neuen Veranstaltung
            neueVeranstaltung = new Strukturelement(veranstaltungName, QUrl(StammURL % urlRaum), courseItem);// % "/materials/documents/"));
            //neueVeranstaltung = new Strukturelement(veranstaltungName, QUrl(StammURL % urlRaum % "/materials/structured/"));
            neueVeranstaltung->setIcon(QIcon(":/Icons/directory"));

            // Hinzufügen der Veranstaltung zur Liste
            itemModel.appendRow(neueVeranstaltung);

            // Weitersetzen der Suchposition hinter den letzten Fund
            altePosition = neuePosition + regExp->matchedLength();
        }



        // Löschen der RegExps aus dem Speicher
        delete regExp;
        delete escapeRegExp;
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
        itemModel.sort(0);

        QObject::disconnect(manager, SIGNAL(finished(QNetworkReply*)),
                            this, SLOT(veranstaltungenAbgerufen(QNetworkReply*)));

        // Aufruf der Funktion zur Aktualisierung der Dateien
        dateienAktualisieren();
    }
}

void Hauptfenster::dateienAktualisieren()
{
    // Prüfen, ob überhaupt Dokumentorte ausgewählt wurden
    if (!ui->documentsCheck->isChecked() && !ui->structeredDocumentsCheck->isChecked() && !ui->exercisesCheck->isChecked())
    {
        // Freischalten von Schaltflächen
        ui->Aktualisieren->setEnabled(true);
        ui->centralwidget->setEnabled(true);
        autoSynchronize = false;
        return;
    }

    QObject::connect(manager, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(dateienAbgerufen(QNetworkReply*)));

    // Durchlaufen aller Veranstaltungen
    int rowCount = itemModel.rowCount();
    Strukturelement* aktuelleStruktur = 0;

    // Übungsbetrieb Lösungen
    // "https://www2.elearning.rwth-aachen.de/ws10/10ws-02568/exerciseCourse/SampleSolutions/"

    // Übungsbetrieb Blätter
    // "https://www2.elearning.rwth-aachen.de/ss11/11ss-33668/exerciseCourse/AssignmentDocuments/

    for(int i= 0; i < rowCount; i++)
    {
        // Holen der aktuellen Veranstaltung
        aktuelleStruktur = (Strukturelement*) itemModel.item(i);

        // Löschen aller Dateien
        if(aktuelleStruktur->rowCount() > 0)
            aktuelleStruktur->removeRows(0, aktuelleStruktur->rowCount());

        // Ausführen des Requests für "Dokumente"
        if (ui->documentsCheck->isChecked())
        {
            // Erstellen eines WebDav Requests
            QNetworkRequest request(QUrl(aktuelleStruktur->data(urlRole).toUrl().toString() % "/materials/documents/"));
            request.setRawHeader("Depth", "infinity");
            request.setRawHeader("Content-Type", "text/xml; charset=\"utf-8\"");
            request.setRawHeader("Content-Length", "0");
            QList<QByteArray> HeaderList = request.rawHeaderList();
            foreach(QByteArray Header, HeaderList)
            {
               qDebug(Header);
            }

            // Einfügen und Absenden des Requests
            replies.insert(manager->sendCustomRequest(request, "PROPFIND"), aktuelleStruktur);
        }

        // Ausführen des Requests für "Strukturierte Materialien"
        if (ui->structeredDocumentsCheck->isChecked())
        {
            // Erstellen eines WebDav Requests
            QNetworkRequest request2(QUrl(aktuelleStruktur->data(urlRole).toUrl().toString() % "/materials/structured/"));
            request2.setRawHeader("Depth", "infinity");
            request2.setRawHeader("Content-Type", "text/xml; charset=\"utf-8\"");
            request2.setRawHeader("Content-Length", "0");

            // Einfügen in die Map und Absenden des Requests
            replies.insert(manager->sendCustomRequest(request2, "PROPFIND"), aktuelleStruktur);
        }

        if (ui->exercisesCheck->isChecked())
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
        ui->Aktualisieren-> setEnabled(true);
        ui->einbinden->     setEnabled(true);
        ui->ausschliessen-> setEnabled(true);
        ui->synchronisieren->setEnabled(true);
        ui->centralwidget-> setEnabled(true);

        autoSynchronize = false;
    }
}

void Hauptfenster::dateienAbgerufen(QNetworkReply* reply)
{
    // Prüfen auf Fehler
    if (!reply->error())
    {
        // Holen die aktuelle Veranstaltung aus der Map
        Strukturelement* aktuellerOrdner = replies.value(reply);

        // Auslesen der Antwort und Speichern in dem XmlReader
        QString replyText = reply->readAll();
        QXmlStreamReader Reader;
        Reader.addData(replyText);

        // Vorbereitung der Daten für die Elemente
        QString currentXmlTag;
        QUrl    url;
        QString name;
        QString time;
        qint32  size = 0;

        // Prüfen auf das Ende
        while(!Reader.atEnd())
        {
            // Lese nächstes Element
            Reader.readNext();

            // 1. Fall: Öffnendes Element <Element>
            if(Reader.isStartElement())
            {
                // Speichern des Namens
                currentXmlTag = Reader.name().toString();
            }

            // 2. Fall: Schließendes Element mit Namen Response </Response>
            else if (Reader.isEndElement() && Reader.name() == "response")
            {
                // Hinzufügen des Slashs bei der Url von Ordnern
                if(!size)
                    url.setUrl(url.toString() % "/");

                // Wechsel in den übergeordneten Ordner des aktuellen Elements
                while(!url.toString().contains((aktuellerOrdner->data(urlRole).toUrl().toString()), Qt::CaseSensitive))//(in = RegExp.indexIn(url.toString())) == -1)
                {
                    aktuellerOrdner->sortChildren(0, Qt::AscendingOrder);
                    aktuellerOrdner = (Strukturelement*)aktuellerOrdner->parent();
                }

                // Ignorieren aller Adressen, die "/Forms" enthalten
                if (!url.toString().contains("/Forms", Qt::CaseSensitive))
                {
                    // Prüfe auf Elementart
                    // 1. Fall: Datei (size > 0)
                    if (size)
                    {
                        // Erstellen einer neuen Datei
                        Datei* newFile = new Datei(name, url, time, size);

                        // Hinzufügen des endungsabhängigen Icons
                        // PDF
                        if (name.contains(QRegExp(".pdf$", Qt::CaseInsensitive)))
                            newFile->setData(QIcon(":/Icons/Icons/filetype_pdf.png"), Qt::DecorationRole);

                        // ZIP
                        else if (name.contains(QRegExp(".zip$", Qt::CaseInsensitive)))
                            newFile->setData(QIcon(":/Icons/Icons/filetype_zip.png"), Qt::DecorationRole);

                        // RAR
                        else if (name.contains(QRegExp(".rar$", Qt::CaseInsensitive)))
                            newFile->setData(QIcon(":/Icons/Icons/filetype_rar.png"), Qt::DecorationRole);

                        // Sonstige
                        else
                            newFile->setData(QIcon(":/Icons/Icons/file.png"), Qt::DecorationRole);


                        QString path;
                        path.append(getStrukturelementPfad(aktuellerOrdner) %"/");
                        path.append(name);
                        path.remove(0,8);

                        if(QFile::exists(path))
                        {
                            newFile->setData(true, synchronisedRole);
                        }

                        // Hinzufügen zum aktuellen Ordner
                        aktuellerOrdner->appendRow(newFile);
                    }
                    // 2. Fall: Ordner/Veranstaltung
                    // Ausschließen der Ordnernamen "documents" und "structured"
                    else if (name != "documents" && name != "structured" && !url.toString().contains("exerciseCourse"))
                    {
                        // Erstellen eines neuen Ordners
                        Strukturelement* neuerOrdner = new Strukturelement(name, url, directoryItem);

                        // Setzen des Zeichens
                        neuerOrdner->setData(QIcon(":/Icons/Icons/25_folder.png"), Qt::DecorationRole);

                        // Hinzufügen zum aktuellen Ordner
                        aktuellerOrdner->appendRow(neuerOrdner);

                        // NeuerOrdner wird zum aktuellen Ordner
                        aktuellerOrdner = neuerOrdner;
                    }
                }

                // Löschen aller eingelesener Daten
                url.clear();
                name.clear();
                size = 0;
                time.clear();
            }

            // Einlesen der Elementeigenschaften
            else if (Reader.isCharacters() && !Reader.isWhitespace())
            {
                // URL
                if(currentXmlTag == "href")
                    url.setUrl(QString::fromUtf8(Reader.text().toString().toLatin1()));

                // Name
                else if (currentXmlTag == "displayname")
                    name = QString::fromUtf8(Reader.text().toString().toLatin1());

                // Größe
                else if (currentXmlTag == "getcontentlength")
                    size = Reader.text().toString().toInt();

                // Modifizierungsdatum
                else if (currentXmlTag == "getlastmodified")
                    time = QString::fromUtf8(Reader.text().toString().toLatin1());
            }
        }

        // Sortieren aller Dateien
        aktuellerOrdner->sortChildren(0, Qt::AscendingOrder);
        replies.value(reply)->sortChildren(0, Qt::AscendingOrder);
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
        ui->Aktualisieren-> setEnabled(true);
        ui->einbinden->     setEnabled(true);
        ui->ausschliessen-> setEnabled(true);
        ui->synchronisieren->setEnabled(true);
        ui->centralwidget-> setEnabled(true);

        // Falls bisher noch nicht synchronisiert wurde und Synchronisation beim Start aktiviert wurde
        if (autoSynchronize)
        {
            autoSynchronize = false;
            on_synchronisieren_clicked();
        }

    }
}

void Hauptfenster::on_ausschliessen_clicked()
{
    // Holen der ausgewählten Dateien
    QModelIndexList ausgewaehlt = proxyModel.mapSelectionToSource(ui->treeView->selectionModel()->selection()).indexes();

    // Iteration über alle Elemente
    Strukturelement* aktuelleStruktur = 0;
    QModelIndexList::Iterator iteratorEnd = ausgewaehlt.end();
    for(QModelIndexList::Iterator iter = ausgewaehlt.begin(); iter != iteratorEnd; iter++)
    {
        // Holen des Vaterelements des ausgewählten Strukturelements
        aktuelleStruktur = (Strukturelement*) iter->internalPointer();

        // Ausschließen des ausgewählten Elements
        Strukturelement* temp = (Strukturelement*)aktuelleStruktur->child(iter->row(),0);
        ausschliessen(temp);
        //ausschliessen((Strukturelement*)aktuelleStruktur->child(iter->row(),0));

        // Prüfen, ob alle Geschwisterelemente ausgeschlossen sind
        // => Ausschließen des Vaterlements
        if(aktuelleStruktur != itemModel.invisibleRootItem())
        {
            bool siblingsExcluded = true;

            // Prüfung aller Zeilen, ob alle ausgeschlossen
            for (int i=0; i < aktuelleStruktur->rowCount(); i++)
            {
                if(((Strukturelement*)aktuelleStruktur->child(i,0))->data(includeRole).toBool())
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
    ui->treeView->dataChanged(itemModel.index(0,0),itemModel.index(itemModel.rowCount(),0));
}

void Hauptfenster::ausschliessen(Strukturelement* aktuelleStruktur)
{
    // Ausschließen aller untergeordneten Elemente
    if(aktuelleStruktur->data(includeRole).toBool()){
        aktuelleStruktur->setData(false, includeRole);
        for(int i = 0; i < aktuelleStruktur->rowCount(); i++)
        {
            ausschliessen((Strukturelement*)aktuelleStruktur->child(i,0));
        }
    }
}

void Hauptfenster::on_einbinden_clicked()
{
    // Bestimmen der ausgewählten Items
    QModelIndexList selectedItemsList =  proxyModel.mapSelectionToSource(ui->treeView->selectionModel()->selection()).indexes();

    // Variablen zur Speicherung von Pointern aus Performancegründen vor der Schleife
    Strukturelement* aktuelleStruktur = 0;
    QModelIndexList::Iterator iteratorEnd = selectedItemsList.end();

    for(QModelIndexList::Iterator iterator = selectedItemsList.begin(); iterator != iteratorEnd; iterator++)
    {
        // Holen des Pointers auf das ausgewählte Item
        aktuelleStruktur = (Strukturelement*)((Strukturelement*) iterator->internalPointer())->child(iterator->row(),0);

        // Einbinden aller übergeordneter Ordner
        Strukturelement* parent = aktuelleStruktur;
        while((parent = (Strukturelement*) parent->parent()) != 0)
        {
            if (parent->data(includeRole).toBool())
                break;
            parent->setData(true, includeRole);
        }

        // Einbinden aller untergeordneter Ordner und Dateien
        einbinden(aktuelleStruktur);
    }

    // Aktualisierung der Ansicht
    ui->treeView->dataChanged(itemModel.index(0,0),itemModel.index(itemModel.rowCount(),0));
}

void Hauptfenster::einbinden(Strukturelement* aktuelleStruktur)
{
    // Einbinden des aktuellen Items
    aktuelleStruktur->setData(true, includeRole);

    // Einbinden aller untergeordnete Ordner und Dateien
    int rowCount = aktuelleStruktur->rowCount();
    for(int i = 0; i < rowCount; i++)
    {
        einbinden((Strukturelement*)aktuelleStruktur->child(i,0));
    }
}

void Hauptfenster::on_Login_clicked()
{
    // Erstellen eines Logintesters
    LoginTester* LoginTest = new LoginTester(ui->BenutzernameFeld->text(), ui->PasswortFeld->text(), 2, this);

    // Testen des Logins
    // 1.Fall: Login erfolgreich
    if(LoginTest->exec())
    {
        // Ändern des Schreibrechts der LineEdits
        ui->BenutzernameFeld->setReadOnly(true);
        ui->PasswortFeld->setReadOnly(true);

        // Hinzufügen eines neuen Styles
        ui->BenutzernameFeld->setStyleSheet("QLineEdit{background-color:#6CFF47; font: bold}");
        ui->PasswortFeld->setStyleSheet("QLineEdit{background-color:#6CFF47; font: bold}");

        // Aktualisieren der Buttons
        ui->Login->setEnabled(false);
        ui->Aktualisieren->setEnabled(true);
        ui->AutoLogin->setEnabled(true);
        ui->DatenSpeichern->setEnabled(true);

        // Setzen des buttons je nach Einstellung
        QSettings einstellungen("Robert K.", "L2P-Tool++");
//        ui->DatenSpeichern->setChecked(einstellungen.value("login/save").toBool());
        ui->AutoLogin->setChecked(einstellungen.value("login/autoLogin").toBool());
        ui->autoSyncCheck->setChecked(einstellungen.value("autoSync").toBool());

        // Anzeigen des Erfolgs in der Statusbar
        ui->statusBar->showMessage("Login erfolgreich!");

        // Sofortiges Aktualisiern der Daten
        on_Aktualisieren_clicked();
    }
    // 2.Fall: Login fehlgeschlagen
    else
    {
        autoSynchronize = false;
    }
    delete LoginTest;
}

void Hauptfenster::on_DatenSpeichern_stateChanged(int checked)
{
    // (De-)Aktivierung der Autologin CB
    if(checked)
        ui->AutoLogin->setEnabled(true);
    else
        ui->AutoLogin->setEnabled(false);

    // Löschen des Hakens
    ui->AutoLogin->setChecked(false);
}

void Hauptfenster::on_AutoLogin_stateChanged(int checked)
{
    // (De-)Aktivierung der Autologin CB
    if(checked)
        ui->autoSyncCheck->setEnabled(true);
    else
        ui->autoSyncCheck->setEnabled(false);

    // Löschen des Hakens
    ui->autoSyncCheck->setChecked(false);
}

void Hauptfenster::on_BenutzernameFeld_textChanged(const QString)
{
    aktiviereLoginButton();
}

void Hauptfenster::on_PasswortFeld_textChanged(const QString)
{
    aktiviereLoginButton();
}

void Hauptfenster::aktiviereLoginButton()
{
    // Löschen der gespeicherten Einstellungen, da neue Benutzerdaten vorliegen
    ui->AutoLogin->setChecked(false);
    ui->DatenSpeichern->setChecked(false);


    // Aktivieren des Loginbuttons, wenn beide Felder ausgefüllt sind
    if(!ui->BenutzernameFeld->text().isEmpty() && !ui->PasswortFeld->text().isEmpty())
    {
        ui->Login->setEnabled(true);
    }
    else
    {
        ui->Login->setEnabled(false);
    }
}

void Hauptfenster::on_synchronisieren_clicked()
{
    ui->centralwidget->setEnabled(false);
    QString directoryPath = ui->VerzeichnisFeld->text();

    // Falls noch kein Pfad angegeben wurde, abbrechen und FileDialog öffnen
    if (directoryPath.isEmpty())
    {
        ui->tabWidget->setCurrentIndex(1);
        on_directoryButton_clicked();
        ui->centralwidget->setEnabled(true);
        return;
    }
    // Deaktivieren des DialogButtons
    ui->directoryButton->setEnabled(false);

    QDir verzeichnis(directoryPath);

    // Überprüfung, ob das angegebene Verzeichnis existiert, oder erstellt werden kann
    if(!verzeichnis.exists() && !verzeichnis.mkpath(verzeichnis.path()))
    {
        ui->directoryButton->setEnabled(true);
        ui->centralwidget->setEnabled(true);
        return;   
    }

    // Hinzufügen aller eingebundenen Elemente einer Liste
    QLinkedList<Strukturelement*> liste;
    for(int i=0; i < itemModel.rowCount(); i++)
    {
        getStrukturelementeListe((Strukturelement*)itemModel.item(i,0), liste, true);
    }

    if(!liste.isEmpty()){

        DateiDownloader* loader = new DateiDownloader(ui->BenutzernameFeld->text(), // Benutzername
                                                      ui->PasswortFeld->text(),     // Passwort
                                                      getFileCount(liste),          // Anzahl der zu runterladenen Dateien
                                                      this);

        // Iterieren über alle Elemente
        Strukturelement* aktuellerOrdner = liste.first();
        int counter = getFileCount(liste);
        int changedCounter = 0;
        bool neueVeranstaltung = false;
        QString veranstaltungName;
        QList<QString> downloadedItems;
        for(QLinkedList<Strukturelement*>::iterator iterator = liste.begin();
            iterator != liste.end();
            iterator++)
        {
            if((**iterator).parent() != 0)
            {
                while(!(**iterator).data(urlRole).toUrl().toString().contains(
                          aktuellerOrdner->data(urlRole).toUrl().toString(), Qt::CaseSensitive))
                {

                    aktuellerOrdner = (Strukturelement*) aktuellerOrdner->parent();
                    verzeichnis.cdUp();
                }
            }
            else
            {
                verzeichnis.setPath(ui->VerzeichnisFeld->text());
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
                         != (*((Datei*)(*iterator))).data(sizeRole).toInt()))
                {
                    if (!loader->startNextDownload((**iterator).text(),
                                                  veranstaltungName,
                                                  verzeichnis.absoluteFilePath((**iterator).text()),
                                                  (**iterator).data(urlRole).toUrl(),
                                                  changedCounter+1))
                        break;
                    changedCounter++;
                    (**iterator).setData(true, synchronisedRole);
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
    ui->directoryButton->setEnabled(true);
    ui->centralwidget->setEnabled(true);


}

void Hauptfenster::getStrukturelementeListe(Strukturelement* aktuellesElement, QLinkedList<Strukturelement*>& liste, bool onlyIncluded)
{
    if(!onlyIncluded || (onlyIncluded && aktuellesElement->data(includeRole).toBool()))
    {
        if ((ui->maxSizeCheckBox->isChecked() && aktuellesElement->data(sizeRole).toInt() < (ui->maxSizeBox->value() * 1024 * 1024))
                || !ui->maxSizeCheckBox->isChecked())
        liste.append(aktuellesElement);
        if(aktuellesElement->hasChildren())
            for (int i = 0; i < aktuellesElement->rowCount(); i++)
                getStrukturelementeListe((Strukturelement*)aktuellesElement->child(i, 0), liste, onlyIncluded);
    }
}

int Hauptfenster::getFileCount(QLinkedList<Strukturelement*>& liste)
{
    // Zählen aller Dateien einer Liste
    int fileCounter = 0;
    for(QLinkedList<Strukturelement*>::iterator iterator = liste.begin(); iterator != liste.end(); iterator++)
    {
        if ((**iterator).type() == fileItem)
            fileCounter++;
    }
    return fileCounter;
}

void Hauptfenster::authentifizieren(QNetworkReply*, QAuthenticator* auth)
{
    auth->setUser(ui->BenutzernameFeld->text());
    auth->setPassword(ui->PasswortFeld->text());
}

void Hauptfenster::unknownError()
{

}

void Hauptfenster::on_directoryButton_clicked()
{
    // Aufruf des Ordnerdialogs
    QString newDirectory = QFileDialog::getExistingDirectory(this,
                                                             "Downloadverkzeichnis auswählen",
                                                             QDir::rootPath(),
                                                             QFileDialog::ShowDirsOnly| QFileDialog::DontResolveSymlinks);
    if (!newDirectory.isEmpty())
        ui->VerzeichnisFeld->setText(newDirectory);

    // Aktualisierung des Zustands des Buttons "Speicherort öffnen"
    if(!ui->VerzeichnisFeld->text().isEmpty())
        ui->directoryOpen->setEnabled(true);
    else
        ui->directoryOpen->setEnabled(false);
}


void Hauptfenster::on_directoryOpen_clicked()
{
    // Betriebssystem soll mit dem Standardprogramm den Pfad öffnen
    QDesktopServices::openUrl(QUrl("file:///" % ui->VerzeichnisFeld->text(), QUrl::TolerantMode));
}

void Hauptfenster::on_expandButton_clicked()
{
    // Expandieren aller Zweige
    ui->treeView->expandAll();
}

void Hauptfenster::on_collapseButton_clicked()
{
    // Reduktion aller Zweige
    ui->treeView->collapseAll();
}

void Hauptfenster::on_treeView_doubleClicked(const QModelIndex &index)
{
    Strukturelement* element = (Strukturelement*) itemModel.itemFromIndex(proxyModel.mapToSource(index));
    if(element->type() == fileItem)
        if(!QDesktopServices::openUrl(QUrl(getStrukturelementPfad(element), QUrl::TolerantMode)))
        {
            QDesktopServices::openUrl(element->data(urlRole).toUrl());
        }
}

QString Hauptfenster::getStrukturelementPfad(Strukturelement* item)
{
    QString path;
    path.append(item->text());
    Strukturelement* parent = item;
    while ((parent = (Strukturelement*) parent->parent()) != 0)
        path.push_front(parent->text() % "/");
    path.push_front("file:///" % ui->VerzeichnisFeld->text() % "/");
    return path;
}

void Hauptfenster::on_treeView_customContextMenuRequested(const QPoint &pos)
{
    // Bestimmung des Elements, auf das geklickt wurde
    Strukturelement* RightClickedItem = (Strukturelement*) itemModel.itemFromIndex(proxyModel.mapToSource(ui->treeView->indexAt(pos)));

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
    QObject::connect(copyAction, SIGNAL(triggered()), this, SLOT(copyURL()));

    // Anzeigen des Menus an der Mausposition
    newCustomContextMenu.exec(ui->treeView->mapToGlobal(pos));
}

void Hauptfenster::openCourse()
{
    // Öffnen der URL des mit der rechten Maustaste geklickten Items
    QDesktopServices::openUrl(lastRightClickItem->data(urlRole).toUrl());
}

void Hauptfenster::openItem()
{
    // Öffnen der Datei auf der Festplatte des mit der rechten Maustaste geklickten Items
    if(!QDesktopServices::openUrl(QUrl(getStrukturelementPfad(lastRightClickItem), QUrl::TolerantMode)))
    {
        // Öffnen der Datei im L2P des mit der rechten Maustaste geklickten Items
        QDesktopServices::openUrl(lastRightClickItem->data(urlRole).toUrl());
    }
}

void Hauptfenster::copyURL()
{
    // Holen der globalen Zwischenablage
    QClipboard *clipboard = QApplication::clipboard();

    // Kopieren der URL des mit der rechten Maustaste geklickten Items in die Zwischenablage
    clipboard->setText(lastRightClickItem->data(urlRole).toUrl().toString());
}


void Hauptfenster::on_maxSizeBox_valueChanged(int newMaximumSize)
{
    proxyModel.setMaximumSize(newMaximumSize);
}

void Hauptfenster::on_maxSizeCheckBox_toggled(bool checked)
{
    proxyModel.setMaximumSizeFilter(checked);
}
