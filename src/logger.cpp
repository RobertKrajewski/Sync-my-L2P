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

    ui->logLevelCB->addItem(tr("Standard"));
    ui->logLevelCB->addItem(tr("Erweitert"));

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
    settings.endGroup();

}

/// Empfänger für alle Nachrichten, die im Log auftauchen sollen
void Logger::logSlot(QString message, int level)
{
    (void) level;

    // Füge eine Leerzeile und färbe Nachrichten entsprechend des Typs ein
    if (message.contains("DEBUG")){
        ui->logList->addItem(message + "\n");
        ui->logList->item(ui->logList->count()-1)->setForeground(Qt::gray);

    }
    else  if (message.contains("ERROR")){
        ui->logList->addItem(message + "\n");
        ui->logList->item(ui->logList->count()-1)->setForeground(Qt::red);

    }
    else
    {
        ui->logList->addItem(message + "\n");
        // Use system default color
    }

}

/// Ausgewählte Logstufe an den Logger weitergeben
void Logger::on_logLevelCB_currentIndexChanged(const QString &logLevel)
{
    if(logLevel == QString(tr("Standard")))
    {
        QsLogging::Logger::instance().setLoggingLevel(QsLogging::InfoLevel);
        QLOG_INFO() << tr("Setze Logging auf \"Standard\".");
    }
    else if(logLevel == QString(tr("Erweitert")))
    {
        QsLogging::Logger::instance().setLoggingLevel(QsLogging::TraceLevel);
        QLOG_INFO() << tr("Setze Logging auf \"Erweitert\".");
    }
}

/// Speichern des gesamten Log in einer Datei
void Logger::on_savePB_clicked()
{
    QString textToWrite = getLogText();

    QString filepath = QFileDialog::getSaveFileName(this,
                                                    tr("Speicherort für das Logfile"),
                                                    "",
                                                    tr("Textdateien (*.txt)"));

    QLOG_DEBUG() << tr("Ausgewählter Speicherort für das Logfile: ") << filepath;

    QFile file(filepath);
    if(!file.open(QIODevice::WriteOnly))
    {
        QLOG_ERROR() << tr("Fehler beim Initialisieren des Logfiles: ") << file.errorString();
        return;
    }

    if(file.write(textToWrite.toLatin1()) == -1)
    {
        QLOG_ERROR() << tr("Fehler beim Schreiben des Logfiles: ") << file.errorString();
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
void Logger::retranslate()
{
    ui->retranslateUi(this);
}
