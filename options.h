#ifndef OPTIONS_H
#define OPTIONS_H

#include <QWidget>
#include <QFileDialog>
#include <QSettings>

#include <QDir>
#include <QFile>

#include "logintester.h"
class Browser;


namespace Ui {
class Options;
}

class Options : public QWidget
{
    Q_OBJECT
    
public:
    explicit Options(QWidget *parent = 0);
    ~Options();
    void loadSettings();
    void saveSettings();
    void updateLoginPushButton(void);

    // Getter
    bool isUserDataSaveCheckBoxChecked();
    bool isDocumentsCheckBoxChecked();
    bool isSharedMaterialsCheckBoxChecked();
    bool isTutorDocumentsCheckBoxChecked();
    bool isExercisesCheckBoxChecked();
    bool isCurrentSemesterCheckBoxChecked();
    bool isOldSemesterCheckBoxChecked();
    bool isAutoLoginOnStartCheckBoxChecked();
    bool isAutoSyncOnStartCheckBoxChecked();
    bool isMinimizeInTrayCheckBoxChecked();
    bool isAutoCloseAfterSyncCheckBoxChecked();
    bool isAutoBackgroundSyncCheckBoxChecked();

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
    
private:
    Ui::Options *ui;

    Browser *browser;

    // Zählvariable für jeden Loginversuch
    int loginCounter;

private slots:
    void on_userDataSaveCheckBox_stateChanged(int);
    void on_userNameLineEdit_textChanged(const QString);
    void on_userPasswordLineEdit_textChanged(const QString);
    void on_downloadFolderPushButton_clicked();
    void on_autoLoginOnStartCheckBox_stateChanged(int arg1);
    void on_downloadFolderlineEdit_textChanged(const QString);
};

#endif // OPTIONS_H
