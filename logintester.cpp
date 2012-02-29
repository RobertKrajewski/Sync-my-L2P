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

#include "logintester.h"
#include "ui_logintester.h"
#include <QSslConfiguration>


LoginTester::LoginTester(QString username,
                         QString password,
                         int maxcount,
                         QWidget *parent) :
    QDialog(parent, Qt::FramelessWindowHint),
    ui(new Ui::LoginTester)
{
    ui->setupUi(this);
    ui->button->hide();

    // Initialisieren der Variablen
    tryCounter = 0;
    this->maxTries      = maxcount;
    this->username      = username;
    this->password      = password;

    // Initialisieren des NetworkManagers und der Slots
    manager = new QNetworkAccessManager(qApp);

//    QSslConfiguration newSslConfiguration = QSslConfiguration::defaultConfiguration();
//    newSslConfiguration.setProtocol(QSsl::SslV3);
//    QSslConfiguration::setDefaultConfiguration(newSslConfiguration);

    QObject::connect(manager, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)), this, SLOT(authenticationSlot(QNetworkReply*, QAuthenticator*)));
    QObject::connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishedSlot(QNetworkReply*)));

    // Starte den Login nach 100ms
    // Bei 0ms wird der Shot unter Mac nicht ausgeführt!?
    QTimer::singleShot(100, this, SLOT(startSlot()));
}

LoginTester::~LoginTester()
{
    delete ui;
}

void LoginTester::authenticationSlot(QNetworkReply* , QAuthenticator* authenticator)
{
    qDebug("authenticationSlot call");
    // Logindaten nur ausfüllen, falls nicht mehr als die maximale Anzahl an Versuchen durchgeführt wurden
    if (tryCounter < maxTries)
    {
        authenticator->setUser(username);
        authenticator->setPassword(password);

    }

    // Erhöhen des Zählers nach jedem Versuch
    tryCounter++;
}

void LoginTester::startSlot()
{
    // Aufruf der L2P-Startseite zum Test der Logindaten
    QNetworkReply* reply = manager->get(QNetworkRequest(QUrl("https://www2.elearning.rwth-aachen.de/foyer/summary/default.aspx")));
    QObject::connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrorsSlot(QList<QSslError>)));
    qDebug("Connection started");
    //reply->ignoreSslErrors();
}

void LoginTester::sslErrorsSlot(QList<QSslError> sslErrors)
{
    qDebug("sslErrorsSlot call");
    foreach(QSslError error, sslErrors)
    {
        qDebug((QString("Error: ") + QString::number(error.error())+ QString(" ")+ error.errorString()).toAscii());
        QSslCertificate certificate = error.certificate();
        qDebug(QString(certificate.isNull()?"NULL ":"NOT NULL ").toAscii());
        qDebug(QString(certificate.isValid()?"VALID ":"NOT VALID ").toAscii());


    }
}

void LoginTester::finishedSlot(QNetworkReply* reply)
{
    qDebug(QString(reply->readAll()).toAscii());
    // Fehlerbehandlung
    if (reply->error())
    {
        QSslConfiguration sslConfiguration = reply->sslConfiguration();
        qDebug(QString("Protocol: " + QString::number(sslConfiguration.protocol())).toAscii());
        //QSslCertificate localCertificate = sslConfiguration.peerCertificate();

        QList<QSslCertificate> sslCertificateList = sslConfiguration.peerCertificateChain();
        foreach(QSslCertificate localCertificate, sslCertificateList){
            qDebug("----------------------------------------------------------");
            qDebug(QString(localCertificate.isNull()?"NULL ":"NOT NULL ").toAscii());
            qDebug(QString(localCertificate.isValid()?"VALID ":"NOT VALID ").toAscii());
            qDebug("\r\nsubjectInfo");
            qDebug(QString("LocalityName: "+localCertificate.subjectInfo(QSslCertificate::LocalityName)).toAscii());
            qDebug(QString("CommonName: "+localCertificate.subjectInfo(QSslCertificate::CommonName)).toAscii());
            qDebug(QString("CountryName: "+localCertificate.subjectInfo(QSslCertificate::CountryName)).toAscii());
            qDebug(QString("Organization: "+localCertificate.subjectInfo(QSslCertificate::Organization)).toAscii());
            qDebug(QString("OrganizationalUnitName: "+localCertificate.subjectInfo(QSslCertificate::OrganizationalUnitName)).toAscii());
            qDebug("\r\nissuerInfo");
            qDebug(QString("LocalityName: "+localCertificate.issuerInfo(QSslCertificate::LocalityName)).toAscii());
            qDebug(QString("CommonName: "+localCertificate.issuerInfo(QSslCertificate::CommonName)).toAscii());
            qDebug(QString("CountryName: "+localCertificate.issuerInfo(QSslCertificate::CountryName)).toAscii());
            qDebug(QString("Organization: "+localCertificate.issuerInfo(QSslCertificate::Organization)).toAscii());
            qDebug(QString("OrganizationalUnitName: "+localCertificate.issuerInfo(QSslCertificate::OrganizationalUnitName)).toAscii());
        }

        QMessageBox messageBox;
        messageBox.setText("Login fehlgeschlagen");
        messageBox.setInformativeText(QString(reply->errorString()));
        messageBox.setDetailedText(QString(reply->readAll()));
        messageBox.setStandardButtons(QMessageBox::Ok);
        messageBox.exec();
        reject();
    }
    else
        accept();
}
