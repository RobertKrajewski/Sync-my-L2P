
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


MyMainWindow::MyMainWindow(QWidget *parent):
    QMainWindow(parent,
                Qt::CustomizeWindowHint | Qt::
                WindowTitleHint | Qt::WindowCloseButtonHint | Qt::
                WindowMinimizeButtonHint), ui(new Ui::MyMainWindow)
{
    // Fenster und Tabs initialisieren
    ui->setupUi(this);
    init();


    QObject::connect(ui->browserTab, SIGNAL(enableSignal(bool)), this, SLOT(enable(bool)));
    QObject::connect(ui->optionsTab, SIGNAL(enableSignal(bool)), this, SLOT(enable(bool)));

    QObject::connect(ui->browserTab, SIGNAL(switchTab(int)), this, SLOT(switchTabSlot(int)));
    QObject::connect(ui->optionsTab, SIGNAL(switchTab(int)), this, SLOT(switchTabSlot(int)));

    QObject::connect(ui->optionsTab, SIGNAL(downloadFolderLineEditTextChanged(QString)), ui->browserTab, SLOT(downloadDirectoryLineEditChangedSlot(QString)));

    QObject::connect(ui->optionsTab, SIGNAL(successfulLogin()), ui->browserTab, SLOT(successfulLoginSlot()));

    // Einstellungen laden
    QCoreApplication::setOrganizationName("Sync-my-L2P");
    QCoreApplication::setOrganizationDomain("Sync-my-L2P.de");
    QCoreApplication::setApplicationName("Sync-my-L2P");
    removeOldSettings();
    loadSettings();

    // Starten der Darstellung des Fensters
    this->show();

    // Zentrieren des Fensters
    centerOnDesktop();

    // Ausführen des Autologins, falls gewünscht
    if (ui->optionsTab->isAutoLoginOnStartCheckBoxChecked())
        ui->optionsTab->on_loginPushButton_clicked();
}

MyMainWindow::~MyMainWindow()
{
    saveSettings();
    delete ui;
}

// Laden der gespeicherten Einstellungen aller Tabs
void MyMainWindow::loadSettings()
{
    ui->browserTab->loadSettings();
    ui->optionsTab->loadSettings();
}

// Speichern aller Einstellungen der Tabs
void MyMainWindow::saveSettings()
{
    ui->browserTab->saveSettings();
    ui->optionsTab->saveSettings();
}

// Löschen aller gespeicherten Einstellungen der alten Version
void MyMainWindow::removeOldSettings()
{
    QSettings settings("Robert K.", "L2P-Tool++");
    settings.clear();
}

// Initialisieren der Tabs
// d.h. übergeben der Pointer der anderen Tabs
void MyMainWindow::init()
{
    ui->browserTab->init(ui->optionsTab);
    ui->optionsTab->init(ui->browserTab); // unnötig? ggf. entfernen
}

void MyMainWindow::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange)
    {
        QWindowStateChangeEvent *e = (QWindowStateChangeEvent*)event;
        // make sure we only do this for minimize events
        if ((e->oldState() != Qt::WindowMinimized) && isMinimized())
        {
            createTrayIcon();
            QTimer::singleShot(0, this, SLOT(hide()));
            event->ignore();
        }
    }
}

void MyMainWindow::createTrayIcon()
{
    trayIcon = new QSystemTrayIcon(QIcon(":/Icons/Icons/magnifier.png"),this);
    trayIcon->show();
    QObject::connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayClickedSlot(QSystemTrayIcon::ActivationReason)));
}

// Zentrieren des Programms auf dem Desktop
// vermutliches Problem: Mehrere Bildschirme
void MyMainWindow::centerOnDesktop()
{
    QRect desktopRect = QApplication::desktop()->screenGeometry();
    QRect windowRect = this->frameGeometry();
    move((desktopRect.width()  - windowRect.width())  / 2,
         (desktopRect.height() - windowRect.height()) / 2);
}

// Dieser Slot wird dazu genutzt, das MainWindow bei längeren Operationen zu blockieren
void MyMainWindow::enable(bool enabled)
{
    this->setEnabled(enabled);
}

void MyMainWindow::switchTabSlot(int tab)
{
    ui->tabWidget->setCurrentIndex(tab);
}

void MyMainWindow::trayClickedSlot(QSystemTrayIcon::ActivationReason reason)
{
    if(isMinimized() && reason == QSystemTrayIcon::Trigger)
    {
        show();
        this->setWindowState(Qt::WindowActive);
        delete trayIcon;
    }
}

