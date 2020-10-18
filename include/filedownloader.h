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

#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QTimer>
#include <QEventLoop>
#include <QStringBuilder>
#include <QFile>
#include <QMessageBox>
#include <QCloseEvent>

#include "utils.h"
#include <sys/types.h>

#ifdef _WIN32
#include <sys/utime.h>
#else
#include <utime.h>
#endif

namespace Ui {
    class DateiDownloader;
}

class FileDownloader : public QDialog
{
    Q_OBJECT

public:
    explicit FileDownloader(int itemNumber,
                             QWidget *parent= nullptr);
    ~FileDownloader();
    int startNextDownload(QString, QString, QString, QUrl, int, int, int time);

private:
    void keyPressEvent(QKeyEvent *);
    Ui::DateiDownloader *ui;
    QNetworkAccessManager* manager;
    QNetworkReply* reply;

    QEventLoop loop;

    int itemNumber;
    bool originalModifiedDate;

    QFile output;
    utimbuf times;

    QString correctUnit(qint64 bytes);
    double correctSize(qint64 bytes);

    QTime downloadTime;

    bool showedError;

private slots:
    void downloadProgressSlot(qint64,qint64);
    void readyReadSlot();
    void finishedSlot();
    void on_abortPushButton_clicked();
};

#endif // DATEIDOWNLOADER_H
