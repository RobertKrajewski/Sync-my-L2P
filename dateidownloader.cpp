#include "dateidownloader.h"
#include "ui_dateidownloader.h"

DateiDownloader::DateiDownloader(QString benutzername,
                                 QString passwort,
                                 int anzahlItems,
                                 QWidget *parent) :
    QDialog(parent, Qt::FramelessWindowHint),
    ui(new Ui::DateiDownloader),
    benutzername(benutzername),
    passwort(passwort),
    anzahlItems(anzahlItems)
{
    ui->setupUi(this);
    manager = new QNetworkAccessManager(this);

    QObject::connect(manager, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*))
                     , this, SLOT(authenticate(QNetworkReply*, QAuthenticator*)));
    this->show();

    // Zentrieren des Fensters
    QRect desktopRect = parentWidget()->frameGeometry();
    QRect windowRect  = this->frameGeometry();
    move((desktopRect.width()-windowRect.width())/2+desktopRect.x(), (desktopRect.height()-windowRect.height())/2+desktopRect.y());
}

DateiDownloader::~DateiDownloader()
{
    delete ui;
}

void DateiDownloader::authenticate(QNetworkReply* , QAuthenticator* authenticator)
{
    authenticator->setUser(benutzername);
    authenticator->setPassword(passwort);
}

int DateiDownloader::startNextDownload(QString dateiname, QString veranstaltung, QString verzeichnisPfad, QUrl url, int itemNummer)
{
    // Anpassen der Labels
    ui->progressLabel->setText(QString("Datei %1/%2").arg(itemNummer).arg(anzahlItems));
    ui->veranstaltungLabel->setText(veranstaltung);
    ui->dateinameLabel->setText(dateiname);

    // Erstellen des Outputstreams
    output.setFileName(verzeichnisPfad);

    // Öffnen des Outputstreams
    if(!output.open(QIODevice::WriteOnly))
    {
        QMessageBox messageBox;
        messageBox.setText("Fehler beim Öffnen mit Schreibberechtigung.");
        messageBox.setInformativeText(dateiname);
        messageBox.setStandardButtons(QMessageBox::Ok);
        messageBox.exec();
        return 0;
    }

    // Start des Requests
    reply = manager->get(QNetworkRequest(url));
    QObject::connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgressSlot(qint64,qint64)));
    QObject::connect(reply, SIGNAL(readyRead()), this, SLOT(readyReadSlot()));
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(finishedSlot()));

    // Schleife, die läuft, bis der Download abgeschlossen ist
    return(loop.exec());
}

void DateiDownloader::downloadProgressSlot(qint64 bytesReceived, qint64 bytesTotal)
{
    if(bytesTotal)
    {
        ui->progressBar->setMaximum(bytesTotal);
        ui->progressBar->setValue(bytesReceived);

    }
    else
    {
        ui->progressBar->setMaximum(0);
        ui->progressBar->setValue(0);
    }
}

void DateiDownloader::readyReadSlot()
{
    if (output.write(reply->readAll()) == -1)
    {
        QMessageBox messageBox;
        messageBox.setText("Beim Schreiben einer Datei auf die Fesplatte ist ein Fehler aufgetreten.");
        messageBox.setInformativeText(ui->dateinameLabel->text());
        messageBox.setStandardButtons(QMessageBox::Ok);
        messageBox.exec();
        reply->abort();
    }
}

void DateiDownloader::finishedSlot()
{
    output.flush();
    output.close();

    QObject::disconnect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgressSlot(qint64,qint64)));
    QObject::disconnect(reply, SIGNAL(readyRead()), this, SLOT(readyReadSlot()));
    QObject::disconnect(reply, SIGNAL(finished()), this, SLOT(finishedSlot()));

    reply->deleteLater();

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
    else
    loop.exit(1);
}

void DateiDownloader::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape)
    {
        event->accept();
        reply->abort();
    }
    else
        event->ignore();
}

