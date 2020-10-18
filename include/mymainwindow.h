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
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QSystemTrayIcon>
#include <QTranslator>

#include "browser.h"

namespace Ui {
    class MyMainWindow;
}

class MyMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MyMainWindow(QWidget *parent = nullptr);
    void closeEvent(QCloseEvent * event);

private slots:
    void trayClickedSlot(QSystemTrayIcon::ActivationReason);
    void on_langCB_currentIndexChanged(const QString &lang);
    void showStatusMessage(QString msg);
    void retranslate();

private:
    void init();

    void changeEvent(QEvent *);

    void createTrayIcon();

    void loadSettings();
    void saveSettings();
    void removeOldSettings();

    void centerOnDesktop();

    void checkForUpdate();

    QSystemTrayIcon *trayIcon;
    Ui::MyMainWindow*      ui;

    QTranslator m_translator;
};

#endif // HAUPTFENSTER_H
