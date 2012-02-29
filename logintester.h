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
#include <QSsl>

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
    void sslErrorsSlot(QList<QSslError>);
};

#endif // LOGINTESTER_H
