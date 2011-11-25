#include <QApplication>
#include "hauptfenster.h"

int main(int argv, char **args)
{
    QApplication qtApplikation(argv, args);
    Hauptfenster programm;
    return qtApplikation.exec();
}
