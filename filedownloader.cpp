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

#include "filedownloader.h"
#include "ui_dateidownloader.h"
#include "math.h"
#include <QDebug>

FileDownloader::FileDownloader(QString username,
                                 QString password,
                                 int itemNumber,
                                 bool originalModifiedDate,
                                 QWidget *parent) :
    QDialog(parent, Qt::FramelessWindowHint),
    ui(new Ui::DateiDownloader),
    username(username),
    password(password),
    originalModifiedDate(originalModifiedDate),
    itemNumber(itemNumber)
{
    ui->setupUi(this);

    manager = new QNetworkAccessManager(this);

    QObject::connect(manager, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*))
                     , this, SLOT(authenticate(QNetworkReply*, QAuthenticator*)));
    this->show();

    // Zentrieren des Fensters
    QRect desktopRect = QApplication::desktop()->screenGeometry();/*parentWidget()->frameGeometry();*/
    QRect windowRect  = this->frameGeometry();
    move((desktopRect.width()-windowRect.width())/2+desktopRect.x(), (desktopRect.height()-windowRect.height())/2+desktopRect.y());
}

FileDownloader::~FileDownloader()
{
    delete ui;
}

void FileDownloader::authenticate(QNetworkReply* , QAuthenticator* authenticator)
{
    authenticator->setUser(username);
    authenticator->setPassword(password);
}

int FileDownloader::startNextDownload(QString filename, QString event, QString verzeichnisPfad, QUrl url, int itemNummer, int itemSize)
{
    // Anpassen der Labels
    // Aktualisieren der Itemnummer
    ui->progressLabel->setText(QString("Datei %1/%2").arg(itemNummer).arg(itemNumber));
    // Aktualisieren des Veranstaltungsnamen
    ui->veranstaltungLabel->setText(event);
    // Aktualisieren des Dateinamens
    ui->dateinameLabel->setText(filename);

    // Erstellen des Outputstreams
    output.setFileName(verzeichnisPfad);

    // Öffnen des Ausgabestreams
    if(!output.open(QIODevice::WriteOnly))
    {
        // Fehlerbehandlung
        Utils::errorMessageBox("Fehler beim Öffnen mit Schreibberechtigung.", filename);
        return 0;
    }

    // Start des Requests
    reply = manager->get(QNetworkRequest(url));
    ui->progressBar->setFormat(QString("%v ").append(dataUnitFromBytes(itemSize)).append(" / %m ").append(dataUnitFromBytes(itemSize)));
    ui->progressBar->setMaximum(roundBytes(itemSize));
    QObject::connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgressSlot(qint64,qint64)));
    QObject::connect(reply, SIGNAL(readyRead()), this, SLOT(readyReadSlot()));
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(finishedSlot()));

    // Starten der Schleife, die vor sich hinläuft, bis der Download abgeschlossen ist
    return(loop.exec());
}

void FileDownloader::downloadProgressSlot(qint64 bytesReceived, qint64 bytesTotal)
{
    // Aktualisieren der Progressbar anhand der Größe der empfangenen Bytes
    ui->progressBar->setValue(roundBytes(bytesReceived));
}

void FileDownloader::readyReadSlot()
{
    // Schreiben der runtergeladenen Bytes in die Datei
    if (output.write(reply->readAll()) == -1)
    {
        Utils::errorMessageBox("Beim Schreiben einer Datei auf die Fesplatte ist ein Fehler aufgetreten.", ui->dateinameLabel->text());
        reply->abort();
    }
}

void FileDownloader::finishedSlot()
{
    // Entleeren und Schließen des Ausgabestreams
    output.flush();
    output.close();

    if (originalModifiedDate)
    {
        times.actime = 0;
        times.modtime = reply->header(QNetworkRequest::LastModifiedHeader).toDateTime().toTime_t();
        utime(output.fileName().toLocal8Bit(), &times);
    }

    QObject::disconnect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgressSlot(qint64,qint64)));
    QObject::disconnect(reply, SIGNAL(readyRead()), this, SLOT(readyReadSlot()));
    QObject::disconnect(reply, SIGNAL(finished()), this, SLOT(finishedSlot()));

    // Freigabe des Speichers
    reply->deleteLater();

    // Fehlerbehandlung
    if(reply->error())
    {
        QMessageBox messageBox;
        messageBox.setText("Beim Download einer Datei ist ein Fehler aufgetreten.");
        messageBox.setInformativeText(ui->dateinameLabel->text());
        messageBox.setDetailedText(reply->errorString());
        messageBox.setStandardButtons(QMessageBox::Ok);
        messageBox.exec();
        output.remove();
        loop.exit(0);
    }
    // Kein Fehler
    else
        loop.exit(1);
}

void FileDownloader::on_abortPushButton_clicked()
{
    keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier));
}

void FileDownloader::keyPressEvent(QKeyEvent *event)
{
    // Abfangen der Escapetaste
    if(event->key() == Qt::Key_Escape)
    {
        // Abbrechen des Synchronisation
        event->accept();
        reply->abort();
    }
    else
        event->ignore();
}

QString FileDownloader::dataUnitFromBytes(qint64 bytes) {
    if (bytes/1024 > 0) {
        return "kB";
    } else {
        return "Byte";
    }
}

qint64 FileDownloader::roundBytes(qint64 bytes) {
    if (bytes/1024 > 0) {
        return (qint64)ceil(((double)bytes/1024));
    } else {
        return bytes;
    }
}
