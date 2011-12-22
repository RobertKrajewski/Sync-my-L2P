#ifndef HAUPTFENSTER_H
#define HAUPTFENSTER_H

#include <QMainWindow>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QAuthenticator>
#include <QUrl>

#include <QRegExp>
#include <QXmlStreamReader>

#include <QStringBuilder>
#include <QList>
#include <QLinkedList>
#include <QQueue>
#include <QSettings>

#include <QMessageBox>
#include <QFileDialog>

#include <QDir>
#include <QFile>

#include <QMenu>
#include <QAction>

#include <QDesktopWidget>
#include <QDesktopServices>

#include "datei.h"

#include "logintester.h"
#include "dateidownloader.h"
#include "mysortfilterproxymodel.h"

class Veranstaltung;

namespace Ui {
    class Hauptfenster;
}

class Hauptfenster : public QMainWindow
{
    Q_OBJECT

public:
    explicit Hauptfenster(QWidget *parent = 0);
    ~Hauptfenster();

private:
    void ausschliessen(Strukturelement*);
    void einbinden(Strukturelement*);
    void aktiviereLoginButton(void);
    void getStrukturelementeListe(Strukturelement*, QLinkedList<Strukturelement*>&, bool);
    void unknownError();

    QString getStrukturelementPfad(Strukturelement*);
    int getFileCount(QLinkedList<Strukturelement*>& liste);

    Ui::Hauptfenster*      ui;
    QNetworkAccessManager* manager;
    MySortFilterProxyModel proxyModel;
    QStandardItemModel     itemModel;
    Strukturelement*       iter;
    QFile                  output;
    QMap<QNetworkReply*, Strukturelement*> replies;


    Strukturelement* lastRightClickItem;
    bool             autoSynchronize;


signals:
    void downloadFortschritt(int);

private slots:
    void copyURL();
    void openItem();
    void openCourse();
    void veranstaltungenAbgerufen(QNetworkReply*);
    void authentifizieren(QNetworkReply*, QAuthenticator*);
    void dateienAbgerufen(QNetworkReply*);
    void dateienAktualisieren();
    void on_ausschliessen_clicked();
    void on_einbinden_clicked();
    void on_Login_clicked();
    void on_DatenSpeichern_stateChanged(int arg1);
    void on_BenutzernameFeld_textChanged(const QString &arg1);
    void on_PasswortFeld_textChanged(const QString &arg1);
    void on_synchronisieren_clicked();
    void on_directoryButton_clicked();
    void on_Aktualisieren_clicked();
    void on_directoryOpen_clicked();
    void on_expandButton_clicked();
    void on_collapseButton_clicked();
    void on_AutoLogin_stateChanged(int arg1);
    void on_treeView_doubleClicked(const QModelIndex &index);
    void on_treeView_customContextMenuRequested(const QPoint &pos);
    void on_maxSizeBox_valueChanged(int arg1);
    void on_maxSizeCheckBox_toggled(bool checked);
};

#endif // HAUPTFENSTER_H
