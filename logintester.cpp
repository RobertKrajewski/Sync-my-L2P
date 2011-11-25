#include "logintester.h"
#include "ui_logintester.h"

LoginTester::LoginTester(QString Benutzername,
                         QString Passwort,
                         int maxcount,
                         QWidget *parent) :
    QDialog(parent, Qt::FramelessWindowHint),
    ui(new Ui::LoginTester)
{
    ui->setupUi(this);
    ui->button->hide();

    counter = 0;
    this->maxcount      = maxcount;
    this->Benutzername  = Benutzername;
    this->Passwort      = Passwort;
    manager = new QNetworkAccessManager(qApp);

    QObject::connect(manager, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)), this, SLOT(authenticate(QNetworkReply*, QAuthenticator*)));
    QObject::connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finish(QNetworkReply*)));
    QTimer::singleShot(100, this, SLOT(start()));
}

LoginTester::~LoginTester()
{
    delete ui;
}

void LoginTester::authenticate(QNetworkReply* , QAuthenticator* authenticator)
{
    if (counter < maxcount)
    {
        authenticator->setUser(Benutzername);
        authenticator->setPassword(Passwort);
    }
    counter++;
}

void LoginTester::start()
{
   manager->get(QNetworkRequest(QUrl("https://www2.elearning.rwth-aachen.de/foyer/summary/default.aspx")));
}

void LoginTester::finish(QNetworkReply* reply)
{
    if (reply->error())
    {
        //ui->button->setText("Login fehlgeschlagen");
        QMessageBox messageBox;
        messageBox.setText("Login fehlgeschlagen");
        messageBox.setInformativeText(reply->errorString());
        messageBox.setDetailedText(QString(reply->readAll()));
        messageBox.setStandardButtons(QMessageBox::Ok);
        messageBox.exec();
        reject();
        //QObject::connect(ui->button, SIGNAL(clicked()), this, SLOT(reject()));
    }
    else
        accept();
//        QObject::connect(ui->button, SIGNAL(clicked()), this, SLOT(accept()));
//    ui->button->show();
}
