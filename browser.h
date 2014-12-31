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
class Veranstaltung;



namespace Ui {
class Browser;
}

class Browser : public QWidget
{
    Q_OBJECT
    
public:
    explicit Browser(QWidget *parent = 0);
    ~Browser();
    void init(Options *options);
    void loadSettings();
    void saveSettings();

public slots:
    void on_refreshPushButton_clicked();
    void downloadDirectoryLineEditChangedSlot(QString downloadDirectory);
    void clearItemModel();

signals:
    void enableSignal(bool);
    void switchTab(int);
    
private:
    void removeSelection(Structureelement*);
    void addSelection(Structureelement*);

    void getStructureelementsList(Structureelement*, QLinkedList<Structureelement*>&, bool);
    void getStructureelementsList(QStandardItem *topElement, QLinkedList <Structureelement *> &list);

    int getFileCount(QLinkedList<Structureelement*>& items);

    void loadStructureelementFromXml(QDomElement item, QStandardItem *parentItem);
    void saveStructureelementToXml(QDomDocument &domDoc, QStandardItem *item, QDomElement *parentItem);

    void updateButtons();

    QNetworkRequest *webdavRequest(Structureelement *aktuelleVeranstaltung, QString urlExtension);

    QNetworkRequest *apiRequest(Structureelement *course, QString apiExtension);

    Ui::Browser *ui;
    QNetworkAccessManager   *manager;
    MySortFilterProxyModel  proxyModel;
    QStandardItemModel      *itemModel;
    QStandardItemModel      *oldItemModel;
    Structureelement        *iter;
    QFile                   output;
    QMap<QNetworkReply*, Structureelement*> replies;


    Structureelement* lastRightClickItem;

    Options *options;

    int refreshCounter;

private slots:
    void openFile();
    void openCourse();
    void coursesRecieved(QNetworkReply*);
    void filesRecieved(QNetworkReply*);
    void requestFileInformation();
    void on_searchPushButton_clicked();
    void on_removeSelectionPushButton_clicked();
    void on_addSelectionPushButton_clicked();
    void on_syncPushButton_clicked();
    void on_openDownloadfolderPushButton_clicked();
    void on_expandPushButton_clicked();
    void on_contractPushButton_clicked();
    void on_dataTreeView_doubleClicked(const QModelIndex &index);
    void on_dataTreeView_customContextMenuRequested(const QPoint &pos);
    void on_sizeLimitSpinBox_valueChanged(int arg1);
    void on_sizeLimitCheckBox_toggled(bool checked);
    void on_dateFilterCheckBox_toggled(bool checked);
    void on_minDateEdit_dateChanged(const QDate &date);
    void on_maxDateEdit_dateChanged(const QDate &date);
    void on_showNewDataPushButton_clicked();
    void copyUrlToClipboardSlot();
    void successfulLoginSlot();
};

#endif // BROWSER_H
