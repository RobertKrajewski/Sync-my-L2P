#include "logintester.h"
#include "ui_logintester.h"

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
    manager->get(QNetworkRequest(QUrl("https://www2.elearning.rwth-aachen.de/foyer/summary/default.aspx")));
}

void LoginTester::finishedSlot(QNetworkReply* reply)
{
    // Fehlerbehandlung
    if (reply->error())
    {
        QMessageBox messageBox;
        messageBox.setText("Login fehlgeschlagen");
        messageBox.setInformativeText(reply->errorString());
        messageBox.setDetailedText(QString(reply->readAll()));
        messageBox.setStandardButtons(QMessageBox::Ok);
        messageBox.exec();
        reject();
    }
    else
        accept();
}
