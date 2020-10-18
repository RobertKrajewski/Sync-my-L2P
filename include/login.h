#ifndef LOGIN_H
#define LOGIN_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QString>
#include <QTimer>

class Login: public QObject
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);

    bool isRefreshTokenAvailable() { return !refreshToken.isEmpty(); }
    void init();
    void saveAccessToken();

signals:
    void newAccessToken(QString accessToken);
    void loginFailed();

public slots:
    void getAccess();
    void stopLoginSlot();
    void deleteAccess();
    void getTokenInfo();

private:
    void postRequest(QUrlQuery &query, QUrl url);


    QNetworkAccessManager manager;

    QJsonObject verification;

    QString refreshToken;
    QString accessToken;

    bool stopLogin;
    QTimer stopLoginTimer;

    bool settingsLoaded;

private slots:
    void finishedSlot(QNetworkReply *reply);
    void checkForVerification();
    void refreshAccess();
    void getUserCode();
};

#endif // LOGIN_H
