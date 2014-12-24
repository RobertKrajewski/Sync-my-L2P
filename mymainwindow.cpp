
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

#include "mymainwindow.h"
#include "ui_mymainwindow.h"

#include "qslog/QsLog.h"
#include "qslog/QsLogDest.h"


MyMainWindow::MyMainWindow(QWidget *parent):
    QMainWindow(parent,
                Qt::CustomizeWindowHint | Qt::
                WindowTitleHint | Qt::WindowCloseButtonHint | Qt::
                WindowMinimizeButtonHint), ui(new Ui::MyMainWindow), trayIcon(NULL)
{
    // Fenster und Tabs initialisieren
    ui->setupUi(this);
    init();

    QsLogging::Logger::instance().addDestination(QsLogging::DestinationFactory::MakeFunctorDestination(this, SLOT(logSlot(QString,int))));
    QsLogging::Logger::instance().setLoggingLevel(QsLogging::DebugLevel);

    QObject::connect(ui->browserTab, SIGNAL(enableSignal(bool)), this, SLOT(setEnabled(bool)));
    QObject::connect(ui->optionsTab, SIGNAL(enableSignal(bool)), this, SLOT(setEnabled(bool)));

    QObject::connect(ui->browserTab, SIGNAL(switchTab(int)), ui->tabWidget, SLOT(setCurrentIndex(int)));
    QObject::connect(ui->optionsTab, SIGNAL(switchTab(int)), ui->tabWidget, SLOT(setCurrentIndex(int)));

    QObject::connect(ui->optionsTab, SIGNAL(downloadFolderLineEditTextChanged(QString)), ui->browserTab, SLOT(downloadDirectoryLineEditChangedSlot(QString)));

    QObject::connect(ui->optionsTab, SIGNAL(successfulLogin()), ui->browserTab, SLOT(successfulLoginSlot()));


    // Gespeicherte Einstellungen für das gesamte Programm laden
    loadSettings();

    // Zeige das Hauptfenster
    this->show();

    // Zentrieren des Fensters
    centerOnDesktop();

    // Erzeugt das Icon für das Tray im minimierten Zustand
    createTrayIcon();

    // Ausführen des Autologins
    if (ui->optionsTab->isAutoLoginOnStartCheckBoxChecked())
    {
        ui->optionsTab->on_loginPushButton_clicked();
    }
}

MyMainWindow::~MyMainWindow()
{
    saveSettings();
    delete ui;
    delete trayIcon;

    QsLogging::Logger::destroyInstance();
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

    ui->browserTab->loadSettings();
    ui->optionsTab->loadSettings();
}

/// Speichern aller Einstellungen
void MyMainWindow::saveSettings()
{
    ui->browserTab->saveSettings();
    ui->optionsTab->saveSettings();
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
            trayIcon->showMessage("Sync-my-L2P", "Läuft im Hintergrund weiter.");
            QTimer::singleShot(0, this, SLOT(hide()));
            event->ignore();
        }
    }
}

void MyMainWindow::createTrayIcon()
{
    trayIcon = new QSystemTrayIcon(QIcon(":/Icons/Icons/magnifier.png"), this);
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

/// Zentrieren des Programms auf dem Desktop
void MyMainWindow::centerOnDesktop()
{
    QRect desktopRect = QApplication::desktop()->screenGeometry();
    QRect windowRect = this->frameGeometry();
    move((desktopRect.width()  - windowRect.width())  / 2,
         (desktopRect.height() - windowRect.height()) / 2);
}

/// Empfänger für alle Nachrichten, die im Log auftauchen sollen
void MyMainWindow::logSlot(QString message, int level)
{
    (void) level;
    ui->logListWidget->addItem(message);
}

