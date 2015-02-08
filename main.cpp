#include <QApplication>
#include <QTranslator>
#include <QLocale>

#include "mymainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QString locale = QLocale::system().name();

        // Lade lokale Sprachdatei oder falle auf Englisch zurück
        QTranslator myappTranslator;
        if(!myappTranslator.load(":/lang/sync-my-l2p_" +locale))
        {
            myappTranslator.load(":/lang/sync-my-l2p_en");
        }
        a.installTranslator(&myappTranslator);

    MyMainWindow w;
    w.show();
    
    return a.exec();
}
