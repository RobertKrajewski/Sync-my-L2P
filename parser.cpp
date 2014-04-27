#include "parser.h"
#include <QDebug>

extern QString MainURL;

Parser::Parser(QObject *parent) :
    QObject(parent)
{
}

void Parser::parseCourses(QNetworkReply *reply, QStandardItemModel *itemModel)
{
    // Auslesen der kompletten Antwort
    QString replyText = reply->readAll();

    // Erstellen eines RegExps für das Herausfiltern der Veranstaltungen
    QString regPattern = "<td class=\"ms-vb2\"><a href=\"(/(?:ws|ss)\\d{2}/\\d{2}(?:ws|ss)-\\d{5})(?:/information/default.aspx)*\">(.*)</a></td><td";
    QRegExp* regExp = new QRegExp(regPattern, Qt::CaseSensitive);
    regExp->setMinimal(true);

    // Erstellen eines RegExps  für unzulässige Buchstaben im Veranstaltungsnamen
    QString escapePattern = "(:|<|>|/|\\\\|\\||\\*|\\^|\\?|\\\")";
    QRegExp* escapeRegExp = new QRegExp(escapePattern, Qt::CaseSensitive);

    // Speichern der Suchpositionen in der Antwort
    int altePosition = 0;
    int neuePosition = 0;

    // neue Veranstaltung sowie Daten
    Structureelement* neueVeranstaltung;
    QString urlRaum;
    QString veranstaltungName;

    // Durchsuchen der gesamten Antwort nach Veranstaltungen
    while((neuePosition=regExp->indexIn(replyText, altePosition)) != -1)
    {
        // Anpassen der Encodierung wegen der Umlaute
        urlRaum = regExp->cap(1);
        veranstaltungName = regExp->cap(2);
        veranstaltungName = veranstaltungName.replace(*escapeRegExp, "").trimmed();


        // Erstellen der neuen Veranstaltung
        neueVeranstaltung = new Structureelement(veranstaltungName, QUrl(MainURL % urlRaum), courseItem);// % "/materials/documents/"));
        //neueVeranstaltung = new Strukturelement(veranstaltungName, QUrl(StammURL % urlRaum % "/materials/structured/"));
        neueVeranstaltung->setIcon(QIcon(":/Icons/directory"));

        // Hinzufügen der Veranstaltung zur Liste
        itemModel->appendRow(neueVeranstaltung);

        // Weitersetzen der Suchposition hinter den letzten Fund
        altePosition = neuePosition + regExp->matchedLength();
    }



    // Löschen der RegExps aus dem Speicher
    delete regExp;
    delete escapeRegExp;
}

void Parser::parseFiles(QNetworkReply *reply, QMap<QNetworkReply*, Structureelement*> *replies, QString downloadDirectoryPath)
{
    // Holen die aktuelle Veranstaltung aus der Map
    Structureelement* aktuellerOrdner = replies->value(reply);

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
            QString bla = url.toString();
            while(!url.toString().contains((aktuellerOrdner->data(urlRole).toUrl().toString()), Qt::CaseSensitive))//(in = RegExp.indexIn(url.toString())) == -1)
            {
                aktuellerOrdner->sortChildren(0, Qt::AscendingOrder);
                aktuellerOrdner = (Structureelement*)aktuellerOrdner->parent();
                if (aktuellerOrdner == 0)
                    qDebug() << replyText;
            }

            // Ignorieren aller Adressen, die "/Forms" enthalten
            if (!url.toString().contains("/Forms", Qt::CaseSensitive))
            {
                // Prüfe auf Elementart
                // 1. Fall: Datei (size > 0)
                if (size)
                {
                    // Erstellen einer neuen Datei
                    Structureelement* newFile = new Structureelement(name, url, time, size);

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
                    path.append(Utils::getStrukturelementPfad(aktuellerOrdner, downloadDirectoryPath) %"/");
                    path.append(name);
                    path.remove(0,8);

                    if(QFile::exists(path))
                    {
                        newFile->setData(SYNCHRONISED, synchronisedRole);
                    }
                    else
                    {
                        newFile->setData(NOT_SYNCHRONISED, synchronisedRole);
                    }

                    QList<QStandardItem*> row;
                    row.append(newFile);
                    row.append(new QStandardItem(QString::number(size/1024.0/1024.0, 'f', 2) % " MB"));
                    row.append(new QStandardItem(QDateTime::fromString(time, Qt::ISODate).toString("yyyy-MM-dd hh:mm")));

                    // Hinzufügen zum aktuellen Ordner
                    aktuellerOrdner->appendRow(row);
                }
                // 2. Fall: Ordner/Veranstaltung
                // Ausschließen der Ordnernamen "documents" und "structured"
                else if (name != "documents" && name != "structured" && !url.toString().contains("exerciseCourse"))
                {
                    // Erstellen eines neuen Ordners
                    Structureelement* newDirectory = new Structureelement(name, url, directoryItem);

                    // Setzen des Zeichens
                    newDirectory->setData(QIcon(":/Icons/Icons/25_folder.png"), Qt::DecorationRole);

                    // Hinzufügen zum aktuellen Ordner
                    aktuellerOrdner->appendRow(newDirectory);

                    // NeuerOrdner wird zum aktuellen Ordner
                    aktuellerOrdner = newDirectory;
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
            if(currentXmlTag == "href" && url.isEmpty())
                url.setUrl(Reader.text().toString());

            // Name
            else if (currentXmlTag == "displayname")
                name = Reader.text().toString();

            // Größe
            else if (currentXmlTag == "getcontentlength")
                size = Reader.text().toString().toInt();

            // Modifizierungsdatum
            else if (currentXmlTag == "getlastmodified")
                time = Reader.text().toString();
        }
    }

    // Leere Ordner wieder rausschmeißen.
    Structureelement* rootCourse = replies->value(reply);
    for (int i = 0; i < rootCourse->rowCount(); i++)
    {
        Structureelement *item = (Structureelement *)rootCourse->child(i);
        if (item->type() == directoryItem && item->rowCount() == 0)
        {
            rootCourse->removeRow(i);
        }
    }
    // Sortieren aller Dateien
    replies->value(reply)->sortChildren(0, Qt::AscendingOrder);
}
