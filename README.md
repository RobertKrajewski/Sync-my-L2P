Sync-my-L2P
===========

Sync-my-L2P ist ein Programm, das von Studenten der RWTH Aachen für den komfortablen Download aller zur Verfügung gestellten Dateien in ihrem Lernportal "L²P" geschaffen wurde. Sie ermöglicht dir, sehr einfach die Daten aus deinen virtuellen Lernräumen mit deiner Festplatte zu spiegeln, so dass diese auch ohne Internet zur Verfügung stehen.

Download
----------------
Sync-my-L2P steht vorgefertigt auf der [offiziellen Webseite](http://www.syncmyl2p.de) oder [hier](https://github.com/Sync-my-L2P/Sync-my-L2P/releases/tag/v2.2.0) für mehrere Plattformen bereit. 


Kompilieren
----------------

Für das Kompilieren benötigt man folgende Bibliotheken: QT (5.4 oder höher), OpenSSL.
Am einfachsten ist es, die .pro Datei in Qt Creator zu laden und dort zu kompilieren. Über die Konsole kann stattdessen auch qmake (mit entsprechenden Argumenten) und dann der bevorzugte C++ Compiler ausgeführt werden.

Zur Erstellung von Linuxpaketen gibt es ein separates Repository: https://github.com/justin-time/Sync-my-L2P-Linux
An dieser Stelle vielen Dank an Stefan!

Achtung: Damit das Programm eine Verbindung mit dem L2P herstellen kann, ist eine ClientID notwendig. Seitens der RWTH ist es verboten, diese zu veröffentlichen.

Installation auf OSX (Cask)
----------------
Users with home-brew installed can now install Sync-my-L2p with the following commands:
`brew tap caskroom/cask` followed by `brew cask install sync-my-l2p`. Updating the program will work with the command `brew update` to update the program formula, followed by `brew upgrade` to upgrade the program itself.


Fehler gefunden?
----------------

Solltest du einen Fehler gefunden haben, füge diesen bitte dem Issue-Tracker hinzu.
