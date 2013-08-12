#include "options.h"
#include "ui_options.h"

#include <QDebug>
#include <QSystemTrayIcon>

#include "browser.h"

Options::Options(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Options)
{
    ui->setupUi(this);
    loginCounter = 0;

    if (QSystemTrayIcon::isSystemTrayAvailable())
        ui->minimizeInTrayCheckBox->setEnabled(true);

    QObject::connect(ui->userPasswordLineEdit, SIGNAL(returnPressed()), ui->loginPushButton, SLOT(click()));
}

Options::~Options()
{
    delete ui;
}

// Laden der lokal gespeicherten Einstellungen für den Tab Einstellungen
void Options::loadSettings()
{
    QSettings settings;

    settings.beginGroup("loginData");
    if (settings.value("saveLoginData", false).toBool())
    {
        ui->userNameLineEdit->setText(              settings.value("u", "").toString());
        ui->userPasswordLineEdit->setText(          settings.value("p", "").toString());
        ui->userDataSaveCheckBox->setChecked(true);
        ui->userDataSaveCheckBox->setEnabled(true);
    }
    else
        ui->userDataSaveCheckBox->setChecked(false);
    settings.endGroup();

    ui->downloadFolderlineEdit->setText(            settings.value("downloadFolder", "").toString());

    settings.beginGroup("semesterFilter");
    ui->currentSemesterCheckBox->setChecked(        settings.value("currentSemester", true).toBool());
    ui->oldSemesterCheckBox->setChecked(            settings.value("oldSemester", false).toBool());
    settings.endGroup();

    settings.beginGroup("documentFilter");
    ui->documentsCheckBox->setChecked(              settings.value("documents", true).toBool());
    ui->sharedMaterialsCheckBox->setChecked(        settings.value("sharedMaterials", true).toBool());
    ui->exercisesCheckBox->setChecked(              settings.value("exercises", true).toBool());
    ui->literatureCheckBox->setChecked(             settings.value("literature", true).toBool());
    ui->tutorDocumentsCheckBox->setChecked(         settings.value("tutorDocuments", true).toBool());
    settings.endGroup();

    settings.beginGroup("automation");
    ui->autoLoginOnStartCheckBox->setChecked(       settings.value("autoLoginOnStart", false).toBool());
    ui->autoSyncOnStartCheckBox->setChecked(        settings.value("autoSyncOnStart", false).toBool());
    ui->autoBackgroundSyncCheckBox->setChecked(     settings.value("autoBackgroundSync", false).toBool());
    ui->autoBackgroundSyncSpinBox->setValue(        settings.value("autoBackgroundSyncTime", 60).toInt());
    ui->autoCloseAfterSyncCheckBox->setChecked(     settings.value("autoCloseAfterSync", false).toBool());
    settings.endGroup();

    settings.beginGroup("misc");
    ui->minimizeInTrayCheckBox->setChecked(         settings.value("minimizeInTray", false).toBool());
    ui->seperateDirectoriesCheckBox->setChecked(    settings.value("seperateDirectories", false).toBool());
    settings.endGroup();
}


// Speichert lokal alle Einstellungen ab, die in unter Einstellungen getroffen wurden
void Options::saveSettings()
{
    QSettings settings;

    if (ui->userDataSaveCheckBox->isChecked())
    {
        settings.beginGroup("loginData");
        settings.setValue("u", ui->userNameLineEdit->text());
        settings.setValue("p", ui->userPasswordLineEdit->text());
        settings.setValue("saveLoginData", true);
        settings.endGroup();
    }
    else
        settings.remove("loginData");

    settings.setValue("downloadFolder",     ui->downloadFolderlineEdit->text());

    settings.beginGroup("semesterFilter");
    settings.setValue("currentSemester",    ui->currentSemesterCheckBox->isChecked());
    settings.setValue("oldSemester",        ui->oldSemesterCheckBox->isChecked());
    settings.endGroup();

    settings.beginGroup("documentFilter");
    settings.setValue("documents",          ui->documentsCheckBox->isChecked());
    settings.setValue("sharedMaterials",    ui->sharedMaterialsCheckBox->isChecked());
    settings.setValue("exercises",          ui->exercisesCheckBox->isChecked());
    settings.setValue("literature",         ui->literatureCheckBox->isChecked());
    settings.setValue("tutorDocuments",     ui->tutorDocumentsCheckBox->isChecked());
    settings.endGroup();

    settings.beginGroup("automation");
    settings.setValue("autoLoginOnStart",   ui->autoLoginOnStartCheckBox->isChecked());
    settings.setValue("autoSyncOnStart",    ui->autoSyncOnStartCheckBox->isChecked());
    settings.setValue("autoBackgroundSync", ui->autoBackgroundSyncCheckBox->isChecked());
    settings.setValue("autoBackgroundSyncTime", ui->autoBackgroundSyncSpinBox->value());
    settings.setValue("autoCloseAfterSync", ui->autoCloseAfterSyncCheckBox->isChecked());
    settings.endGroup();

    settings.beginGroup("misc");
    settings.setValue("minimizeInTray",     ui->minimizeInTrayCheckBox->isChecked());
    settings.setValue("seperateDirectories",ui->seperateDirectoriesCheckBox->isChecked());
    settings.endGroup();
}

void Options::on_loginPushButton_clicked()
{
    loginCounter++;

    // Erstellen eines Logintesters
    LoginTester *loginTester = new LoginTester(ui->userNameLineEdit->text(),
                                               ui->userPasswordLineEdit->text(),
                                               2,
                                               this);

    // Testen des Logins
    // 1.Fall: Login erfolgreich
    if (loginTester->exec())
    {
        // Ändern des Schreibrechts der LineEdits
        ui->userNameLineEdit->setReadOnly(true);
        ui->userPasswordLineEdit->setReadOnly(true);

        // Hinzufügen eines neuen Styles
        ui->userNameLineEdit->setStyleSheet
        ("QLineEdit{background-color:#6CFF47; font: bold}");
        ui->userPasswordLineEdit->setStyleSheet
        ("QLineEdit{background-color:#6CFF47; font: bold}");

        // Aktualisieren der Buttons
        ui->loginPushButton->setEnabled(false);
        //ui->refreshPushButton->setEnabled(true);
        ui->autoLoginOnStartCheckBox->setEnabled(true);
        ui->userDataSaveCheckBox->setEnabled(true);

        // Setzen des buttons je nach Einstellung
        QSettings settings;
        // ui->userDataSaveCheckBox->setChecked(einstellungen.value("login/save").toBool());
        //ui->autoLoginOnStartCheckBox->setChecked(settings.value("login/autoLoginOnStartCheckBox").toBool());
        //ui->autoSyncOnStartCheckBox->setChecked(settings.value("autoSync").toBool());
        // Anzeigen des Erfolgs in der Statusbar
        //ui->statusBar->showMessage("Login erfolgreich!");

        emit switchTab(0);
        // Sofortiges Aktualisiern der Daten
        browser->on_refreshPushButton_clicked();
    }

    delete loginTester;
}

void Options::on_userDataSaveCheckBox_stateChanged(int checked)
{
    // (De-)Aktivierung der autoLoginOnStartCheckBox CB
    if (checked)
        ui->autoLoginOnStartCheckBox->setEnabled(true);
    else
    {
        ui->autoLoginOnStartCheckBox->setEnabled(false);
        ui->autoLoginOnStartCheckBox->setChecked(false);
    }
}

void Options::on_autoLoginOnStartCheckBox_stateChanged(int checked)
{
    // (De-)Aktivierung der autoLoginOnStartCheckBox CB
    if (checked)
    {
        ui->autoSyncOnStartCheckBox->setEnabled(true);
//        ui->autoBackgroundSyncCheckBox->setEnabled(true);
//        ui->autoBackgroundSyncSpinBox->setEnabled(true);
    }
    else
    {
        ui->autoSyncOnStartCheckBox->setEnabled(false);
        ui->autoSyncOnStartCheckBox->setChecked(false);
        ui->autoBackgroundSyncCheckBox->setEnabled(false);
        ui->autoBackgroundSyncCheckBox->setChecked(false);
        ui->autoBackgroundSyncSpinBox->setEnabled(false);
    }
}

void Options::on_downloadFolderlineEdit_textChanged(const QString downloadFolder)
{
    emit downloadFolderLineEditTextChanged(downloadFolder);
}

void Options::on_userNameLineEdit_textChanged(const QString)
{
    updateLoginPushButton();
}

void Options::on_userPasswordLineEdit_textChanged(const QString)
{
    updateLoginPushButton();
}

void Options::updateLoginPushButton()
{
    // Löschen der gespeicherten Einstellungen, da neue Benutzerdaten
    // vorliegen
    //ui->autoLoginOnStartCheckBox->setChecked(false);
    //ui->userDataSaveCheckBox->setChecked(false);

    // Aktivieren des Loginbuttons, wenn beide Felder ausgefüllt sind
    if (!ui->userNameLineEdit->text().isEmpty()
        && !ui->userPasswordLineEdit->text().isEmpty())
    {
        ui->loginPushButton->setEnabled(true);
    }
    else
    {
        ui->loginPushButton->setEnabled(false);
    }
}



void Options::on_downloadFolderPushButton_clicked()
{
    // Aufruf des Ordnerdialogs
    QString newDirectory = QFileDialog::getExistingDirectory(this,
                           "Downloadverkzeichnis auswählen",
                           QDir::rootPath(),
                           QFileDialog::ShowDirsOnly |
                           QFileDialog::DontResolveSymlinks);

    if (!newDirectory.isEmpty())
        ui->downloadFolderlineEdit->setText(newDirectory);
}

QString Options::downloadFolderLineEditText()
{
    return ui->downloadFolderlineEdit->text();
}

QString Options::userNameLineEditText()
{
    return ui->userNameLineEdit->text();
}

QString Options::userPasswordLineEditText()
{
    return ui->userPasswordLineEdit->text();
}

void Options::init(Browser *browser)
{
    this->browser = browser;
}

bool Options::isTutorDocumentsCheckBoxChecked()
{
    return ui->tutorDocumentsCheckBox->isChecked();
}

bool Options::isExercisesCheckBoxChecked()
{
    return ui->exercisesCheckBox->isChecked();
}

bool Options::isLiteratureCheckBoxChecked()
{
    return ui->literatureCheckBox->isChecked();
}
bool Options::isCurrentSemesterCheckBoxChecked()
{
    return ui->currentSemesterCheckBox->isChecked();
}

bool Options::isOldSemesterCheckBoxChecked()
{
    return ui->oldSemesterCheckBox->isChecked();
}

bool Options::isSharedMaterialsCheckBoxChecked()
{
    return ui->sharedMaterialsCheckBox->isChecked();
}

bool Options::isDocumentsCheckBoxChecked()
{
    return ui->documentsCheckBox->isChecked();
}

bool Options::isAutoSyncOnStartCheckBoxChecked()
{
    return ui->autoSyncOnStartCheckBox->isChecked();
}

bool Options::isMinimizeInTrayCheckBoxChecked()
{
    return ui->minimizeInTrayCheckBox->isChecked();
}

bool Options::isAutoCloseAfterSyncCheckBoxChecked()
{
    return ui->autoCloseAfterSyncCheckBox->isChecked();
}

bool Options::isAutoLoginOnStartCheckBoxChecked()
{
    return ui->autoLoginOnStartCheckBox->isChecked();
}

int Options::getLoginCounter()
{
    return loginCounter;
}
