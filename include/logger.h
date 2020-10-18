#ifndef LOGGER_H
#define LOGGER_H

#include <QWidget>

namespace Ui {
class Logger;
}

class Logger : public QWidget
{
    Q_OBJECT

public:
    explicit Logger(QWidget *parent = nullptr);
    ~Logger();

    void saveSettings();

    void loadSettings();

public slots:
    void retranslate();

private slots:
    void logSlot(QString message, int level);

    void on_logLevelCB_currentIndexChanged(const QString &logLevel);

    void on_savePB_clicked();

    void on_copyPB_clicked();

private:
    QString getLogText();

    Ui::Logger *ui;
};

#endif // LOGGER_H
