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

#ifndef DATEIDOWNLOADER_H
#define DATEIDOWNLOADER_H

#include <QDesktopWidget>
#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QAuthenticator>
#include <QTimer>
#include <QEventLoop>
#include <QStringBuilder>
#include <QFile>
#include <QMessageBox>
#include <QCloseEvent>

#include <utils.h>
#include <sys/types.h>
#include <utime.h>

namespace Ui {
    class DateiDownloader;
}

class FileDownloader : public QDialog
{
    Q_OBJECT

public:
    explicit FileDownloader(QString username,
                             QString password,
                             int itemNumber,
                             bool originalModifiedDate,
                             QWidget *parent= 0);
    ~FileDownloader();
    int startNextDownload(QString, QString, QString, QUrl, int, int);

private:
    void keyPressEvent(QKeyEvent *);
    Ui::DateiDownloader *ui;
    QNetworkAccessManager* manager;
    QNetworkReply* reply;

    QEventLoop loop;

    QString username;
    QString password;

    int itemNumber;
    bool originalModifiedDate;

    QFile output;
    utimbuf times;

    QString dataUnitFromBytes(qint64 bytes);
    qint64 roundBytes(qint64 bytes);

private slots:
    void authenticate(QNetworkReply*, QAuthenticator*);
    void downloadProgressSlot(qint64,qint64);
    void readyReadSlot();
    void finishedSlot();
    void on_abortPushButton_clicked();
};

#endif // DATEIDOWNLOADER_H
