#ifndef DATEIDOWNLOADER_H
#define DATEIDOWNLOADER_H

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

namespace Ui {
    class DateiDownloader;
}

class DateiDownloader : public QDialog
{
    Q_OBJECT

public:
    explicit DateiDownloader(QString username,
                             QString password,
                             int itemNumber,
                             QWidget *parent= 0);
    ~DateiDownloader();
    int startNextDownload(QString, QString, QString, QUrl, int);

private:
    void keyPressEvent(QKeyEvent *);
    Ui::DateiDownloader *ui;
    QNetworkAccessManager* manager;
    QNetworkReply* reply;

    QEventLoop loop;

    QString username;
    QString password;

    int itemNumber;

    QFile output;

private slots:
    void authenticate(QNetworkReply*, QAuthenticator*);
    void downloadProgressSlot(qint64,qint64);
    void readyReadSlot();
    void finishedSlot();
};

#endif // DATEIDOWNLOADER_H
