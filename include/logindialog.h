#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>

#include <QNetworkAccessManager>

#include "login.h"

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = 0);
    ~LoginDialog();

public slots:
    void run(Login *login);

private slots:
    void availabilitySlot(QNetworkReply*);
    void failedSlot();
    void succededSlot();

private:
    void checkForAuthentification();

    Ui::LoginDialog *ui;
    QNetworkAccessManager manager;

    // Pointer to Class responsible for the login
    Login *login;
};

#endif // LOGINDIALOG_H
