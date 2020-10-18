#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>

#include <QNetworkAccessManager>

#include "login.h"

enum Availability {
    NOTTESTED,
    AVAILABLE,
    NOTAVAILABLE
};

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

public slots:
    void run(Login *login);

private slots:
    void checkL2PAvailability();
    void checkMoodleAvailability();
    void availabilityL2PSlot(QNetworkReply*);
    void availabilityMoodleSlot(QNetworkReply*);
    void failedSlot();
    void succededSlot();

private:
    void checkForAuthentification();

    Ui::LoginDialog *ui;
    QNetworkAccessManager manager;

    // Pointer to Class responsible for the login
    Login *login;
    Availability l2pAvailable;
    Availability moodleAvailable;
};

#endif // LOGINDIALOG_H
