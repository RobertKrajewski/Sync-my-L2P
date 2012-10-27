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


#include "logintester.h"
#include "filedownloader.h"
#include "mysortfilterproxymodel.h"

#include "parser.h"
#include "utils.h"

class Veranstaltung;

namespace Ui {
    class MyMainWindow;
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
    void ausschliessen(Structureelement*);
    void einbinden(Structureelement*);
    void aktiviereLoginButton(void);
    void getStrukturelementeListe(Structureelement*, QLinkedList<Structureelement*>&, bool);
    void unknownError();


    int getFileCount(QLinkedList<Structureelement*>& liste);

    Ui::MyMainWindow*      ui;
    QNetworkAccessManager* manager;
    MySortFilterProxyModel proxyModel;
    QStandardItemModel     *itemModel;
    Structureelement*       iter;
    QFile                  output;
    QMap<QNetworkReply*, Structureelement*> replies;


    Structureelement* lastRightClickItem;
    bool             autoSynchronize;


signals:
    void downloadFortschritt(int);

private slots:
    void openItem();
    void openCourse();
    void veranstaltungenAbgerufen(QNetworkReply*);
    void doAuthentification(QNetworkReply*, QAuthenticator*);
    void dateienAbgerufen(QNetworkReply*);
    void dateienAktualisieren();
    void on_searchPushButton_clicked();
    void on_removeSelectionPushButton_clicked();
    void on_addSelectionPushButton_clicked();
    void on_loginPushButton_clicked();
    void on_userDataSaveCheckBox_stateChanged(int);
    void on_userNameLineEdit_textChanged(const QString);
    void on_userPasswordLineEdit_textChanged(const QString);
    void on_syncPushButton_clicked();
    void on_downloadFolderPushButton_clicked();
    void on_refreshPushButton_clicked();
    void on_openDownloadfolderPushButton_clicked();
    void on_expandPushButton_clicked();
    void on_contractPushButton_clicked();
    void on_autoLoginOnStartCheckBox_stateChanged(int arg1);
    void on_dataTreeView_doubleClicked(const QModelIndex &index);
    void on_dataTreeView_customContextMenuRequested(const QPoint &pos);
    void on_sizeLimitSpinBox_valueChanged(int arg1);
    void on_sizeLimitCheckBox_toggled(bool checked);
    void on_dateFilterCheckBox_toggled(bool checked);
    void on_minDateEdit_dateChanged(const QDate &date);
    void on_maxDateEdit_dateChanged(const QDate &date);
    void copyUrlToClipboardSlot();
};

#endif // HAUPTFENSTER_H
