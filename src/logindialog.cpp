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
    ui->retranslateUi(this);

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
    QUrl url("https://www3.elearning.rwth-aachen.de/_vti_bin/L2PServices/api.svc/v1/Documentation");
    QNetworkRequest request;
    request.setUrl(url);

    QObject::connect(&manager, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(availabilitySlot(QNetworkReply*)));

    QLOG_INFO() << tr("Erreichbarkeitsrequest");
    manager.get(request);
}

void LoginDialog::availabilitySlot(QNetworkReply * reply)
{
    QObject::disconnect(&manager, SIGNAL(finished(QNetworkReply*)),
                        this, SLOT(availabilitySlot(QNetworkReply*)));

    QString response = reply->readAll();

    if( reply->error() )
    {
        response.truncate( 1000 );
        QLOG_ERROR() << tr("L2P nicht erreichbar. Genauer Fehler: ") << reply->errorString();
        QLOG_ERROR() << tr("Inhalt der Antwort: ") << response;
        ui->statusLabel->setText(tr("Fehler: L2P nicht erreichbar."));
    }
    else
    {
        QLOG_INFO() << tr("L2P erreichbar");
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


