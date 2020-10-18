#ifndef OPTIONS_H
#define OPTIONS_H

#include <QWidget>
#include <QFileDialog>
#include <QSettings>

#include <QDir>
#include <QFile>

#include "login.h"
class Browser;


namespace Ui {
class Options;
}

class Options : public QWidget
{
    Q_OBJECT
    
public:
    explicit Options(QWidget *parent = nullptr);
    ~Options();
    void loadSettings();
    void saveSettings();

    // Getter
    bool isUserDataSaveCheckBoxChecked();
    bool isLearningMaterialsCheckBoxChecked();
    bool isSharedLearningmaterialsCheckBoxChecked();
    bool isAssignmentsCheckBoxChecked();
    bool isMediaLibrarysCheckBoxChecked();
    bool isEmailAttachmentsCheckBoxChecked();
    bool isAnnouncementAttachmentsCheckBoxChecked();
    bool isAutoLoginOnStartCheckBoxChecked();
    bool isAutoSyncOnStartCheckBoxChecked();
    bool isMinimizeInTrayCheckBoxChecked();
    bool isAutoCloseAfterSyncCheckBoxChecked();
    bool isAutoBackgroundSyncCheckBoxChecked();
    bool isOverrideFilesCheckBoxChecked();
    bool isCheckForUpdateCheckBoxChecked();
    bool isCurrentSemesterCheckBoxChecked();
    bool isTutorDomainCheckBoxChecked();

    QString getAccessToken() const;

    int getLoginCounter();

    QString downloadFolderLineEditText();
    QString userNameLineEditText();
    QString userPasswordLineEditText();

    void init(Browser *browser);

signals:
    void enableSignal(bool);
    void switchTab(int);
    void downloadFolderLineEditTextChanged(const QString text);
    void successfulLogin();

public slots:
    void on_loginPushButton_clicked();
    void retranslate();
    void on_aboutButton_clicked();
    
private:
    Ui::Options *ui;

    Browser *browser;

    // Zählvariable für jeden Loginversuch
    int loginCounter;

    Login login;

    QString accessToken;

private slots:
    void on_userDataSaveCheckBox_stateChanged(int);
    void on_downloadFolderPushButton_clicked();
    void on_autoLoginOnStartCheckBox_stateChanged(int arg1);
    void on_downloadFolderlineEdit_textChanged(const QString);
    void on_loginErasePushButton_clicked();
    void loginResultSlot(int result);
    void accessTokenChanged(QString newAccessToken);
};

#endif // OPTIONS_H
