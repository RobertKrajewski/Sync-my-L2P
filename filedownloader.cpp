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

#include "qslog/QsLog.h"

FileDownloader::FileDownloader(int itemNumber,
                                 QWidget *parent) :
    QDialog(parent, Qt::FramelessWindowHint),
    ui(new Ui::DateiDownloader),
    originalModifiedDate(originalModifiedDate),
    itemNumber(itemNumber),
    manager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);
    ui->retranslateUi(this);
    this->show();

    Utils::centerWidgetOnDesktop(this);
}

FileDownloader::~FileDownloader()
{
    delete manager;
    delete ui;
}

int FileDownloader::startNextDownload(QString fileName, QString courseName, QString filePath, QUrl fileUrl, int itemNummer, int itemSize, int time)
{
    // Anpassen der Labels
    ui->progressLabel->setText(QString("Datei %1/%2").arg(itemNummer).arg(itemNumber));
    ui->veranstaltungLabel->setText(courseName);
    ui->dateinameLabel->setText(fileName);
    ui->progressBar->setFormat(QString("%v ") % correctUnit(itemSize) % " / %m " % correctUnit(itemSize));
    ui->progressBar->setMaximum(correctSize(itemSize));
    ui->downloadSpeedLabel->setText("");

    times.actime = 0;
    times.modtime = time;

    // Erstellen des Outputstreams
    output.setFileName(filePath);

    // Öffnen des Ausgabestreams
    if(!output.open(QIODevice::WriteOnly))
    {
        Utils::errorMessageBox(tr("Fehler beim Öffnen mit Schreibberechtigung."), fileName);
        return 0;
    }

    downloadTime.start();

    // Start des Requests
    reply = manager->get(QNetworkRequest(fileUrl));
    QObject::connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgressSlot(qint64,qint64)));
    QObject::connect(reply, SIGNAL(readyRead()), this, SLOT(readyReadSlot()));
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(finishedSlot()));

    // Während des Downloads blockieren
    return(loop.exec());
}

/// Anzeige des Downloadfortschritts der aktuellen Datei
void FileDownloader::downloadProgressSlot(qint64 bytesReceived, qint64 bytesTotal)
{
    (void) bytesTotal;

    // Aktualisieren der Progressbar anhand der Größe der empfangenen Bytes
    ui->progressBar->setValue(correctSize(bytesReceived));
    ui->progressBar->update();

    // Downloadgeschwindigkeit in kB/s
    int downloadSpeed = bytesReceived * 1000 / downloadTime.elapsed() / 1024;
    ui->downloadSpeedLabel->setText(QString::number(downloadSpeed) % " kB/s");
}

/// Abspeichern von empfangenen Dateiteilen
void FileDownloader::readyReadSlot()
{
    // Schreiben der runtergeladenen Bytes in die Datei
    if (output.write(reply->readAll()) == -1)
    {
        Utils::errorMessageBox(tr("Fehler beim Schreiben der Datei"), ui->dateinameLabel->text());
        reply->abort();
    }
}

void FileDownloader::finishedSlot()
{
    // Entleeren und Schließen des Ausgabestreams
    output.flush();
    output.close();

    utime(output.fileName().toLocal8Bit(), &times);

    QObject::disconnect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgressSlot(qint64,qint64)));
    QObject::disconnect(reply, SIGNAL(readyRead()), this, SLOT(readyReadSlot()));
    QObject::disconnect(reply, SIGNAL(finished()), this, SLOT(finishedSlot()));

    // Freigabe des Speichers
    reply->deleteLater();

    if(reply->error())
    {
        Utils::errorMessageBox(tr("Beim Download der Datei %1 ist ein Fehler aufgetreten.").arg(output.fileName()),
                               reply->errorString() % "; " % reply->readAll());
        output.remove();
        loop.exit(0);
    }
    else
    {
        loop.exit(1);
    }
}

void FileDownloader::on_abortPushButton_clicked()
{
    keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier));
}

/// Abbruch der Synchronisation durch die Escapetaste
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
    {
        event->ignore();
    }
}

/// Erzeugung der passenden Größeneinheit von der Dateigröße
QString FileDownloader::correctUnit(qint64 bytes)
{
    if(bytes > 1024 * 1024 * 5)
    {
        return "MB";
    }
    else if (bytes > 1024 * 5)
    {
        return "kB";
    }
    else
    {
        return "Byte";
    }
}

/// Dateigröße in lesbare Größe umwandeln
qint64 FileDownloader::correctSize(qint64 bytes)
{
    if(bytes > 1024 * 1024 * 5)
    {
        return bytes / (1024 * 1024);
    }
    else if (bytes > 1024 * 5)
    {
        return bytes / 1024;
    }
    else
    {
        return bytes;
    }
}
