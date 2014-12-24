#include <QUrlQuery>
#include <QJsonDocument>
#include <QDesktopServices>

#include <QSettings>
#include "clientId.h"
#include "qslog/QsLog.h"
#include "login.h"

#define BASEURL QString("https://oauth.campus.rwth-aachen.de/oauth2waitress/oauth2.svc/")

Login::Login(QWidget *parent)
    : stopLogin(false), settingsLoaded(false), QObject((QObject*)parent)
{
    // Automatisch Abbruch des Logins nach 60 Sekunden
    stopLoginTimer.setSingleShot(true);
    stopLoginTimer.setInterval(60 * 1000);
    QObject::connect(&stopLoginTimer, SIGNAL(timeout()), this, SLOT(stopLoginSlot()));

    QObject::connect( &manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishedSlot(QNetworkReply*)) );
}

void Login::init()
{
    if(settingsLoaded)
    {
        return;
    }

    settingsLoaded = true;

    QSettings settings;

    settings.beginGroup("loginData");
    refreshToken = settings.value("refreshToken", "").toString();
    settings.endGroup();
}

void Login::getAccess()
{
    // Nach 60 Sekunden abbrechen
    stopLogin = false;

    stopLoginTimer.stop();
    stopLoginTimer.start();

    if(refreshToken.isEmpty())
    {
        getUserCode();
    }
    else
    {
        refreshAccess();
    }
}

void Login::postRequest(QUrlQuery &query, QUrl url)
{
    QByteArray queryString;
    queryString.append(query.toString());

    url.setQuery(query);

    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    if(!stopLogin)
    {
        manager.post(request, queryString);
    }
}

void Login::saveAccessToken()
{
    QSettings settings;

    settings.beginGroup("loginData");
    settings.setValue("refreshToken", refreshToken);
    settings.endGroup();
}

void Login::getUserCode()
{
    QUrlQuery query;
    query.addQueryItem("client_id", CLIENTID);
    query.addQueryItem("scope", "l2p2013.rwth userinfo.rwth");

    QUrl url(BASEURL + "code");

    postRequest(query, url);
}

void Login::stopLoginSlot()
{
    stopLoginTimer.stop();

    if(accessToken.isEmpty())
    {
        stopLogin = true;
        emit loginFailed();
    }
}

void Login::deleteAccess()
{
    accessToken.clear();
    refreshToken.clear();
}

void Login::checkForVerification()
{
    // Starte das Polling
    QUrlQuery query;
    query.addQueryItem("client_id", CLIENTID);
    query.addQueryItem("code", verification["device_code"].toString());
    query.addQueryItem("grant_type", "device");

    QUrl url(BASEURL + "token");

    postRequest(query, url);
}

void Login::refreshAccess()
{
    accessToken.clear();

    QUrlQuery query;
    query.addQueryItem("client_id", CLIENTID);
    query.addQueryItem("refresh_token", refreshToken);
    query.addQueryItem("grant_type", "refresh_token");

    QUrl url(BASEURL + "token");

    postRequest(query, url);
}

void Login::finishedSlot(QNetworkReply *reply)
{
    QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
    QJsonObject object = document.object();

    if(object.isEmpty())
    {
        QLOG_DEBUG() << "Keine lesbare Antwort erhalten.";
        stopLoginSlot();
        return;
    }

    QString status = object["status"].toString();
    if(status == QString("ok"))
    {
        if(!object["user_code"].toString().isEmpty())
        {
            // Benutzer muss App Zugriff erlauben

            verification = object;

            QString verificationUrl = verification["verification_url"].toString();
            verificationUrl.append("?q=verify&d=");
            verificationUrl.append(verification["user_code"].toString());

            QUrl url;
            url.setUrl(verificationUrl);

            QLOG_DEBUG() << "Öffne Browser für Verfikation. Url: " << url;
            QDesktopServices::openUrl(url);

            QTimer::singleShot(verification["interval"].toInt() * 1000, this, SLOT(checkForVerification()));
        }
        else if(!object["refresh_token"].toString().isEmpty())
        {
            // Zugriff gewährt

            QLOG_DEBUG() << "Neuer Zugriff gewährt.";
            refreshToken = object["refresh_token"].toString();
            accessToken = object["access_token"].toString();

            QTimer::singleShot(object["expires_in"].toInt() * 1000, this, SLOT(refreshAccess()));

            stopLoginTimer.stop();
            emit newAccessToken(accessToken);
            QLOG_DEBUG() << "Accesstoken: " << accessToken;
        }
        else if(!object["access_token"].toString().isEmpty())
        {
            // Zugriff erneuert

            QLOG_DEBUG() << "Zugriff durch Refreshtoken erneuert.";
            accessToken = object["access_token"].toString();

            QTimer::singleShot(object["expires_in"].toInt() * 1000, this, SLOT(refreshAccess()));

            stopLoginTimer.stop();
            emit newAccessToken(accessToken);
            QLOG_DEBUG() << "Accesstoken: " << accessToken;
        }
        else
        {
            QLOG_DEBUG() << "Status der Antwort ok, aber Antworttyp nicht bekannt.\n" << object;

            stopLoginSlot();
        }
    }
    else if(status == QString("error: authorization pending."))
    {
        QLOG_DEBUG() << status;
        QTimer::singleShot(verification["interval"].toInt() * 1000, this, SLOT(checkForVerification()));
    }
    else if(status == QString("error: slow down"))
    {
        QLOG_DEBUG() << status;
        QTimer::singleShot(verification["interval"].toInt() * 1200, this, SLOT(checkForVerification()));
    }
    else if(status == QString("authorization invalid."))
    {
        // RefreshToken abgelaufen, neuen Authorisierung starten
        QLOG_DEBUG() << status;
        QTimer::singleShot(50, this, SLOT(getUserCode()));
    }
    else if(status == QString("error: refresh token invalid."))
    {
        // Ungültiger RefreshToken
        QLOG_DEBUG() << status;

        QTimer::singleShot(50, this, SLOT(getUserCode()));
    }
    else
    {
        QLOG_DEBUG() << "Unerwarteter Antwortstatus: " << status;

        stopLoginSlot();
    }
}


