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
    explicit LoginTester(QString username,
                         QString password,
                         int maxTries,
                         QWidget *parent = 0);
    ~LoginTester();

private:
    Ui::LoginTester *ui;
    QNetworkAccessManager* manager;

    int tryCounter;
    int maxTries;

    QString username;
    QString password;

private slots:
    void authenticationSlot(QNetworkReply*, QAuthenticator*);
    void finishedSlot(QNetworkReply*);
    void startSlot(void);
};

#endif // LOGINTESTER_H
