#include <QApplication>
#include <QTranslator>
#include <QLocale>

#include "mymainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QString locale = QLocale::system().name();

        QTranslator myappTranslator;
        myappTranslator.load(":/lang/sync-my-l2p_" +locale);
        a.installTranslator(&myappTranslator);

    MyMainWindow w;
    w.show();
    
    return a.exec();
}
