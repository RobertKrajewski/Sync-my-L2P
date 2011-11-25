#ifndef LOGINTESTER_H
#define LOGINTESTER_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QAuthenticator>
#include <QTimer>
#include <QMessageBox>
#include <QList>
#include <QSslError>

namespace Ui {
    class LoginTester;
}

class LoginTester : public QDialog
{
    Q_OBJECT

public:
    explicit LoginTester(QString Benutername,
                         QString Passwort,
                         int maxcount,
                         QWidget *parent = 0);
    ~LoginTester();

private:
    Ui::LoginTester *ui;
    QNetworkAccessManager* manager;

    int counter;
    int maxcount;

    QString Benutzername;
    QString Passwort;

private slots:
    void authenticate(QNetworkReply*, QAuthenticator*);
    void finish(QNetworkReply*);
    void start(void);
};

#endif // LOGINTESTER_H
