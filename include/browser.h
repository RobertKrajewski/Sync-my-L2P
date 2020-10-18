#ifndef BROWSER_H
#define BROWSER_H

#include <QWidget>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QAuthenticator>
#include <QUrl>

#include <QRegExp>
#include <QDomDocument>

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

#include "filedownloader.h"
#include "mysortfilterproxymodel.h"

#include "autoclosedialog.h"

#include "parser.h"
#include "utils.h"

class Options;
class L2pItemModel;

namespace Ui {
class Browser;
}

class Browser : public QWidget
{
    Q_OBJECT
    
public:
    explicit Browser(QWidget *parent = nullptr);
    ~Browser();
    void init(Options *options);
    void loadSettings();
    void saveSettings();

public slots:
    void on_refreshPushButton_clicked();
    void downloadDirectoryLineEditChangedSlot(QString downloadDirectory);
    void clearItemModel();
    void retranslate();

signals:
    void enableSignal(bool);
    void switchTab(int);
    void showStatusMessage(QString);
    
private:
    void removeSelection(Structureelement*);
    void addSelection(Structureelement*);

    void getStructureelementsList(Structureelement*, QList<Structureelement*>&, bool);
    void getStructureelementsList(QStandardItem *topElement, QList <Structureelement *> &list);

    int getFileCount(QList<Structureelement*>& items);

    void updateButtons();
    void setupSignalsSlots();

    Ui::Browser *ui;
    QNetworkAccessManager   *manager;
    MySortFilterProxyModel *proxy;
    Structureelement        *iter;
    QFile                   output;

    Structureelement* lastRightClickItem;

    Options *options;

    int refreshCounter = 0;

    L2pItemModel *l2pItemModel;

private slots:
    void openFile();
    void openMessage();
    void openSourceMessage();
    void openCourse();
    void on_searchPushButton_clicked();
    void on_removeSelectionPushButton_clicked();
    void on_addSelectionPushButton_clicked();
    void on_syncPushButton_clicked();
    void on_openDownloadfolderPushButton_clicked();
    void on_dataTreeView_doubleClicked(const QModelIndex &index);
    void on_dataTreeView_customContextMenuRequested(const QPoint &pos);
    void on_showNewDataPushButton_clicked();
    void copyUrlToClipboardSlot();
    void successfulLoginSlot();
    void itemModelReloadedSlot();
};

#endif // BROWSER_H
