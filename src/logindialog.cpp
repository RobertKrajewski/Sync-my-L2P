#include <QNetworkRequest>
#include <QNetworkReply>

#include "logindialog.h"
#include "urls.h"
#include "ui_logindialog.h"
#include "qslog/QsLog.h"

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    ui->retranslateUi(this);

    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    this->l2pAvailable = NOTTESTED;
    this->moodleAvailable = NOTTESTED;
}

LoginDialog::~LoginDialog()
{
    QObject::disconnect(login, SIGNAL(newAccessToken(QString)), this, SLOT(succededSlot()));
    QObject::disconnect(login, SIGNAL(loginFailed()), this, SLOT(failedSlot()));

    delete ui;
}

void LoginDialog::checkL2PAvailability()
{
    QUrl url(l2pApiDocs);
    QNetworkRequest request;
    request.setUrl(url);

    QObject::connect(&manager, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(availabilityL2PSlot(QNetworkReply*)));

    QLOG_INFO() << tr("L2P Erreichbarkeitsrequest");
    manager.get(request);
}

void LoginDialog::checkMoodleAvailability()
{
    QUrl url(moodleApiDocs);
    QNetworkRequest request;
    request.setUrl(url);

    QObject::connect(&manager, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(availabilityMoodleSlot(QNetworkReply*)));

    QLOG_INFO() << tr("Moodle Erreichbarkeitsrequest");
    manager.get(request);
}

void LoginDialog::run(Login *login)
{
    this->login = login;

    QObject::connect(this, SIGNAL(rejected()), this->login, SLOT(stopLoginSlot()));

    // Überprüfe Erreichbarkeit des L2P
    this->checkL2PAvailability();
    // TODO: check if this method and its slots are right. Keep in mind, that both
    // (availabilityL2PSlot, availabilityMoodleSlot) execute checkForAuthentification
    this->checkMoodleAvailability();
}

void LoginDialog::availabilityL2PSlot(QNetworkReply * reply)
{
    QObject::disconnect(&manager, SIGNAL(finished(QNetworkReply*)),
                        this, SLOT(availabilityL2PSlot(QNetworkReply*)));

    QString response = reply->readAll();

    if( reply->error() )
    {
        response.truncate( 1000 );
        QLOG_ERROR() << tr("L2P nicht erreichbar. Genauer Fehler: ") << reply->errorString();
        QLOG_ERROR() << tr("Inhalt der Antwort: ") << response;
        ui->statusLabel->setText(tr("Fehler: L2P nicht erreichbar."));
        this->l2pAvailable = NOTAVAILABLE;
    }
    else
    {
        QLOG_INFO() << tr("L2P erreichbar");
        this->l2pAvailable = AVAILABLE;
        checkForAuthentification();
    }
}

void LoginDialog::availabilityMoodleSlot(QNetworkReply * reply)
{
    QObject::disconnect(&manager, SIGNAL(finished(QNetworkReply*)),
                        this, SLOT(availabilityMoodleSlot(QNetworkReply*)));

    QString response = reply->readAll();

    if( reply->error() )
    {
        response.truncate( 1000 );
        QLOG_ERROR() << tr("Moodle nicht erreichbar. Genauer Fehler: ") << reply->errorString();
        QLOG_ERROR() << tr("Inhalt der Antwort: ") << response;
        ui->statusLabel->setText(tr("Fehler: Moodle nicht erreichbar."));
        this->moodleAvailable = NOTAVAILABLE;
    }
    else
    {
        QLOG_INFO() << tr("Moodle erreichbar");
        this->moodleAvailable = AVAILABLE;
        checkForAuthentification();
    }
}

void LoginDialog::failedSlot()
{
    ui->statusLabel->setText(tr("Login fehlgeschlagen."));
}

void LoginDialog::succededSlot()
{
    ui->progressBar->setValue(3);
    ui->statusLabel->setText(tr("Login erfolgreich abgeschlossen!"));

    QTimer::singleShot(1500, this, SLOT(accept()));
}

void LoginDialog::checkForAuthentification()
{
    if (this->l2pAvailable == NOTTESTED || this->moodleAvailable == NOTTESTED)
    {
        return;
    }
    QObject::connect(login, SIGNAL(newAccessToken(QString)), this, SLOT(succededSlot()));
    QObject::connect(login, SIGNAL(loginFailed()), this, SLOT(failedSlot()));

    if(!login->isRefreshTokenAvailable())
    {
        ui->progressBar->setValue(1);
        ui->statusLabel->setText(tr("Authentifizierung notwendig. Browser öffnet automatisch."));
        QTimer::singleShot(3000, login, SLOT(getAccess()));
    }
    else
    {
        ui->progressBar->setValue(2);
        ui->statusLabel->setText(tr("Einloggen..."));
        QTimer::singleShot(300, login, SLOT(getAccess()));
    }
}


