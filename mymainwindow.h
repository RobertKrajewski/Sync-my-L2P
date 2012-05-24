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

#include "myfile.h"

#include "logintester.h"
#include "filedownloader.h"
#include "mysortfilterproxymodel.h"

class Veranstaltung;

namespace Ui {
    class MainWindow;
}

class MyMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MyMainWindow(QWidget *parent = 0);
    ~MyMainWindow();

private:
    void loadSettings();
    void saveSettings();
    void ausschliessen(Strukturelement*);
    void einbinden(Strukturelement*);
    void aktiviereLoginButton(void);
    void getStrukturelementeListe(Structureelement*, QLinkedList<Structureelement*>&, bool);
    void unknownError();

    QString getStrukturelementPfad(Structureelement*);
    int getFileCount(QLinkedList<Structureelement*>& liste);

    Ui::MainWindow*      ui;
    QNetworkAccessManager* manager;
    MySortFilterProxyModel proxyModel;
    QStandardItemModel     itemModel;
    Structureelement*       iter;
    QFile                  output;
    QMap<QNetworkReply*, Structureelement*> replies;


    Structureelement* lastRightClickItem;
    bool             autoSynchronize;


signals:
    void downloadFortschritt(int);

private slots:
    void copyURL();
    void openItem();
    void openCourse();
    void veranstaltungenAbgerufen(QNetworkReply*);
    void doAuthentification(QNetworkReply*, QAuthenticator*);
    void dateienAbgerufen(QNetworkReply*);
    void dateienAktualisieren();
    void on_ausschliessen_clicked();
    void on_einbinden_clicked();
    void on_Login_clicked();
    void on_DatenSpeichern_stateChanged(int);
    void on_BenutzernameFeld_textChanged(const QString);
    void on_PasswortFeld_textChanged(const QString);
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
