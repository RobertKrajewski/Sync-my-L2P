#include <QSettings>
#include <QFileDialog>

#include "logger.h"
#include "ui_logger.h"

#include "qslog/QsLog.h"
#include "qslog/QsLogDest.h"

#include "utils.h"

Logger::Logger(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Logger)
{
    ui->setupUi(this);

    ui->logLevelCB->addItem("Standard");
    ui->logLevelCB->addItem("Erweitert");

    QsLogging::Logger::instance().addDestination(QsLogging::DestinationFactory::MakeFunctorDestination(this, SLOT(logSlot(QString,int))));
}

Logger::~Logger()
{
    delete ui;
}

/// Laden der Einstellungen
void Logger::loadSettings()
{
    QSettings settings;
    settings.beginGroup("logger");
    ui->logLevelCB->setCurrentText(settings.value("logLevel", "standard").toString());
    settings.endGroup();
}

/// Speichern der Einstellungen
void Logger::saveSettings()
{
    QSettings settings;
    settings.beginGroup("logger");
    settings.setValue("logLevel", ui->logLevelCB->currentText());
}

/// Empfänger für alle Nachrichten, die im Log auftauchen sollen
void Logger::logSlot(QString message, int level)
{
    (void) level;
    ui->logList->addItem(message);
}

/// Ausgewählte Logstufe an den Logger weitergeben
void Logger::on_logLevelCB_currentIndexChanged(const QString &logLevel)
{
    if(logLevel == QString("Standard"))
    {
        QsLogging::Logger::instance().setLoggingLevel(QsLogging::InfoLevel);
        QLOG_INFO() << "Setze Logging auf \"Standard\".";
    }
    else if(logLevel == QString("Erweitert"))
    {
        QsLogging::Logger::instance().setLoggingLevel(QsLogging::TraceLevel);
        QLOG_INFO() << "Setze Logging auf \"Erweitert\".";
    }
}

/// Speichern des gesamten Log in einer Datei
void Logger::on_savePB_clicked()
{
    QString textToWrite = getLogText();

    QString filepath = QFileDialog::getSaveFileName(this,
                                                    "Speicherort für das Logfile",
                                                    "",
                                                    "Textdateien (*.txt)");

    QLOG_DEBUG() << "Ausgewählter Speicherort für das Logfile: " << filepath;

    QFile file(filepath);
    if(!file.open(QIODevice::WriteOnly))
    {
        QLOG_ERROR() << "Fehler beim initialisieren des Logfiles: " << file.errorString();
        return;
    }

    if(file.write(textToWrite.toLatin1()) == -1)
    {
        QLOG_ERROR() << "Fehler beim Schreiben des Logfiles: " << file.errorString();
        return;
    }

    file.close();
}

/// Kopieren des gesamten Logs in die Zwischenablage
void Logger::on_copyPB_clicked()
{
    Utils::copyTextToClipboard(getLogText());
}

/// Wandelt alle Logs in einen einzigen String um
QString Logger::getLogText()
{
    QList<QListWidgetItem*> items = ui->logList->findItems("*", Qt::MatchWildcard);

    QString logText("");

    foreach(QListWidgetItem* item, items)
    {
        logText.append( item->text() + "\r\n");
    }

    return logText;
}
