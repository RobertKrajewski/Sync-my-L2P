
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

#include <QLocale>
#include <QTranslator>
#include "mymainwindow.h"
#include "ui_mymainwindow.h"

#include "qslog/QsLog.h"
#include "utils.h"


MyMainWindow::MyMainWindow(QWidget *parent):
    QMainWindow(parent), trayIcon(nullptr), ui(new Ui::MyMainWindow)
{
    // Sprache installieren
    QString locale = QLocale::system().name();

    if(!m_translator.load(":/lang/sync-my-l2p_" +locale))
    {
        m_translator.load(":/lang/sync-my-l2p_en");
    }
    qApp->installTranslator(&m_translator);

    // Fenster und Tabs initialisieren
    ui->setupUi(this);
    init();

    QObject::connect(ui->browserTab, SIGNAL(enableSignal(bool)), this, SLOT(setEnabled(bool)));
    QObject::connect(ui->optionsTab, SIGNAL(enableSignal(bool)), this, SLOT(setEnabled(bool)));

    QObject::connect(ui->browserTab, SIGNAL(switchTab(int)), ui->tabWidget, SLOT(setCurrentIndex(int)));
    QObject::connect(ui->optionsTab, SIGNAL(switchTab(int)), ui->tabWidget, SLOT(setCurrentIndex(int)));

    QObject::connect(ui->optionsTab, SIGNAL(downloadFolderLineEditTextChanged(QString)), ui->browserTab, SLOT(downloadDirectoryLineEditChangedSlot(QString)));

    QObject::connect(ui->optionsTab, SIGNAL(successfulLogin()), ui->browserTab, SLOT(successfulLoginSlot()));

    QObject::connect(ui->browserTab, SIGNAL(showStatusMessage(QString)), this, SLOT(showStatusMessage(QString)));

    // Gespeicherte Einstellungen für das gesamte Programm laden
    loadSettings();

    // Zeige das Hauptfenster
    this->show();

    // Zentrieren des Fensters
    Utils::centerWidgetOnDesktop(this);

    // Erzeugt das Icon für das Tray im minimierten Zustand
    createTrayIcon();

    // Prüfen auf eine neue Programmversion
    if(ui->optionsTab->isCheckForUpdateCheckBoxChecked())
    {
        checkForUpdate();
    }

    // Ausführen des Autologins
    if (ui->optionsTab->isAutoLoginOnStartCheckBoxChecked())
    {
        ui->optionsTab->on_loginPushButton_clicked();
    }
}

void MyMainWindow::closeEvent(QCloseEvent * event)
{
    saveSettings();
    QsLogging::Logger::destroyInstance();

    event->accept();
}

/// Laden der gespeicherten Einstellungen aller Tabs
void MyMainWindow::loadSettings()
{
    // Globale Werte setzen
    QCoreApplication::setOrganizationName("Sync-my-L2P");
    QCoreApplication::setOrganizationDomain("Sync-my-L2P.de");
    QCoreApplication::setApplicationName("Sync-my-L2P");

    // Einstellungen alter Versionen entfernen
    removeOldSettings();

    ui->logTab->loadSettings();
    ui->optionsTab->loadSettings();
    ui->browserTab->loadSettings();

    // Beim ersten Start Anleitung anzeigen
    QSettings settings;
    bool firstUse = settings.value("firstUse", "true").toBool();
    if(firstUse)
    {
        ui->tabWidget->setCurrentIndex(3);
    }
}

/// Speichern aller Einstellungen
void MyMainWindow::saveSettings()
{
    ui->browserTab->saveSettings();
    ui->optionsTab->saveSettings();
    ui->logTab->saveSettings();

    QSettings settings;
    settings.setValue("firstUse", "false");
}

/// Löschen alter Einstellungen
void MyMainWindow::removeOldSettings()
{
    QSettings settings0("Robert K.", "L2P-Tool++");
    settings0.clear();

    QSettings settings1;
    settings1.remove("loginData/u");
    settings1.remove("loginData/p");
    settings1.remove("semesterFilter");
}

void MyMainWindow::checkForUpdate()
{
    int currentVersion = 20402;

    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl("https://www.syncmyl2p.de/version.txt"));
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    QEventLoop newLoop;
    QNetworkReply *reply = manager.get(request);
    reply->ignoreSslErrors();

    // Wait for reply
    QObject::connect( reply, SIGNAL(finished()), &newLoop, SLOT(quit()));
    QTimer killTimer;
    killTimer.setSingleShot(true);
    killTimer.start(5000);
    QObject::connect( &killTimer, SIGNAL(timeout()), &newLoop, SLOT(quit()));
    newLoop.exec();

    if( reply->error() )
    {
        QLOG_ERROR() << tr("Konnte Version nicht überprüfen:\n") << reply->errorString();
        return;
    }
    QString replyMessage(reply->readAll());
    QLOG_DEBUG() << tr("Aktuelle Version laut Server:") << replyMessage;
    if( replyMessage.toInt() > currentVersion )
    {
        // Aufhübschen der Versionsnummer
        replyMessage.insert(4, ".");
        replyMessage.insert(2, ".");

        // Anzeige für den Benutzer
        auto button = QMessageBox::question(this,
                                            tr("Neue Version verfügbar!") + " (v" + replyMessage +")",
                                            tr("Auf der offiziellen Webseite ist eine neue Version verfügbar!\n"
                                               "Diese Nachricht kannst du in den Optionen deaktivieren.\n"
                                               "Jetzt https://www.syncmyl2p.de/ aufrufen?"));
        if (button == QMessageBox::Yes)
        {
            QDesktopServices::openUrl(QUrl("https://www.syncmyl2p.de/"));
        }
    }
    else
    {
        QLOG_INFO() << tr("Diese Version ist aktuell") << " (" << currentVersion << ")";
    }
}

/// Tabs Pointer auf die Geschwistertabs übergeben
void MyMainWindow::init()
{
    ui->browserTab->init(ui->optionsTab);
    ui->optionsTab->init(ui->browserTab);
}

/// Minimieren abfangen und ggf. Trayicon erzeugen
void MyMainWindow::changeEvent(QEvent *event)
{

    QWidget::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange)
    {
        QWindowStateChangeEvent *e = (QWindowStateChangeEvent*)event;
        // make sure we only do this for minimize events
        if ((e->oldState() != Qt::WindowMinimized) &&
                isMinimized() &&
                ui->optionsTab->isMinimizeInTrayCheckBoxChecked() &&
                QSystemTrayIcon::isSystemTrayAvailable())
        {
            trayIcon->show();
            trayIcon->showMessage("Sync-my-L2P", tr("Läuft im Hintergrund weiter."));
            QTimer::singleShot(0, this, SLOT(hide()));
            event->ignore();
        }
    }
}

void MyMainWindow::createTrayIcon()
{
    trayIcon = new QSystemTrayIcon(QIcon(":/icons/magnifier.png"), this);
    QObject::connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayClickedSlot(QSystemTrayIcon::ActivationReason)));
}

/// Beim Klick auf das TrayIcon, dieses ausblenden und Programm einblenden
void MyMainWindow::trayClickedSlot(QSystemTrayIcon::ActivationReason reason)
{
    if(isMinimized() && reason == QSystemTrayIcon::Trigger)
    {
        show();
        this->setWindowState(Qt::WindowActive);
        trayIcon->hide();
    }
}

// Installiert die neue Übersetzung, wenn eine andere Sprache gewählt wurde
// Falls neue Sprachen ergänzt werden sollen, müssen diese hier und in der options.cpp ergänzt werden.
void MyMainWindow::on_langCB_currentIndexChanged(const QString &lang){
    QLOG_INFO() << tr("wechsle Sprache auf ") << lang;

    qApp->removeTranslator(&m_translator);
    if (lang == tr("Systemsprache"))
    {
        if(!m_translator.load("sync-my-l2p_" + QLocale::system().name(), ":/lang"))
        {
            m_translator.load("sync-my-l2p_en", ":/lang");
        }
    }
    else if (lang == "Deutsch")
        m_translator.load("sync-my-l2p_de", ":/lang");
    else if (lang == "English")
       m_translator.load("sync-my-l2p_en", ":/lang");
    else if (lang == "Lëtzebuergesch")
        m_translator.load("sync-my-l2p_lb", ":/lang");
    else if (lang == "Shqip")
        m_translator.load("sync-my-l2p_sq", ":/lang");
    else
        m_translator.load("sync-my-l2p_en", ":/lang");


    qApp->installTranslator(&m_translator);
    retranslate();
}

void MyMainWindow::showStatusMessage(QString msg)
{
    ui->statusBar->showMessage(msg);
}

// Läd die neuen Übersetzungen für die GUI Elemente.
void MyMainWindow::retranslate()
{
    ui->retranslateUi(this);
    ui->optionsTab->retranslate();
    ui->browserTab->retranslate();
    ui->logTab->retranslate();
}
