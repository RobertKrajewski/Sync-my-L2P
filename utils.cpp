#include "utils.h"

Utils::Utils(QObject *parent) :
    QObject(parent)
{
}

QString Utils::getStrukturelementPfad(Structureelement *item, QString downloadDirectoryPath)
{
        QString elementPath;
        elementPath.append(item->text());
        Structureelement* parent = item;
        while ((parent = (Structureelement*) parent->parent()) != 0)
            elementPath.push_front(parent->text() % "/");
        elementPath.push_front("file:///" % downloadDirectoryPath % "/");
        return elementPath;
}

void Utils::copyTextToClipboard(Structureelement *item)
{
    // Holen der globalen Zwischenablage
    QClipboard *clipboard = QApplication::clipboard();

    // Kopieren der URL des mit der rechten Maustaste geklickten Items in die Zwischenablage
    clipboard->setText(item->data(urlRole).toUrl().toString());
}
