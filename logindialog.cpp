#include <QNetworkRequest>
#include <QNetworkReply>

#include "logindialog.h"
#include "ui_logindialog.h"
#include "qslog/QsLog.h"

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
}

LoginDialog::~LoginDialog()
{
    QObject::disconnect(login, SIGNAL(newAccessToken(QString)), this, SLOT(succededSlot()));
    QObject::disconnect(login, SIGNAL(loginFailed()), this, SLOT(failedSlot()));

    delete ui;
}

void LoginDialog::run(Login *login)
{
    this->login = login;

    QObject::connect(this, SIGNAL(rejected()), this->login, SLOT(stopLoginSlot()));

    // Überprüfe Erreichbarkeit des L2P
    QUrl url("https://www3.elearning.rwth-aachen.de/l2p/start/SitePages/Start.aspx");
    QNetworkRequest request;
    request.setUrl(url);


    QObject::connect(&manager, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(availabilitySlot(QNetworkReply*)));

    QLOG_INFO() << "Erreichbarkeitsrequest";
    manager.get(request);
}

void LoginDialog::availabilitySlot(QNetworkReply * reply)
{
    QLOG_INFO() << "Erreichbarkeit festgestellt";
    QObject::disconnect(&manager, SIGNAL(finished(QNetworkReply*)),
                        this, SLOT(availabilitySlot(QNetworkReply*)));

    QString response = reply->readAll();

    if( reply->error() )
    {
        QLOG_DEBUG() << "Fehler: L2P nicht erreichbar. Genauer Fehler: " << reply->errorString();
    }

    if(response.contains("die die Kommunikation vereinfachen und verschiedene Assessment-Optionen bieten"))
    {
        checkForAuthentification();
    }
    else
    {
        ui->statusLabel->setText("Fehler: L2P nicht erreichbar.");
    }
}

void LoginDialog::failedSlot()
{
    ui->statusLabel->setText("Login fehlgeschlagen.");
}

void LoginDialog::succededSlot()
{
    ui->progressBar->setValue(3);
    ui->statusLabel->setText("Login erfolgreich abgeschlossen!");

    QTimer::singleShot(1500, this, SLOT(accept()));
}

void LoginDialog::checkForAuthentification()
{
    QObject::connect(login, SIGNAL(newAccessToken(QString)), this, SLOT(succededSlot()));
    QObject::connect(login, SIGNAL(loginFailed()), this, SLOT(failedSlot()));

    if(!login->isRefreshTokenAvailable())
    {
        ui->progressBar->setValue(1);
        ui->statusLabel->setText("Authentifizierung notwendig. Browser öffnet automatisch.");
        QTimer::singleShot(3000, login, SLOT(getAccess()));
    }
    else
    {
        ui->progressBar->setValue(2);
        ui->statusLabel->setText("Einloggen...");
        QTimer::singleShot(300, login, SLOT(getAccess()));
    }
}


