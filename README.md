Sync-my-L2P
===========

Sync-my-L2P ist ein Programm, das von Studenten der RWTH Aachen für den komfortablen Download aller zur Verfügung gestellten Dateien in ihrem Lernportal "L²P" geschaffen wurde. Sie ermöglicht dir, sehr einfach die Daten aus deinen virtuellen Lernräumen mit deiner Festplatte zu spiegeln, so dass diese auch ohne Internet zur Verfügung stehen.

Kompilieren
----------------

Für das Kompilieren benötigt man folgende Bibliotheken: QT (5.4 oder höher), OpenSSL.
Am einfachsten ist es, die .pro Datei in Qt Creator zu laden und dort zu kompilieren. Über die Konsole kann stattdessen auch qmake (mit entsprechenden Argumenten) und dann der bevorzugte C++ Compiler ausgeführt werden.

Achtung: Damit das Programm eine Verbindung mit dem L2P herstellen kann, ist eine ClientID notwendig. Seitens der RWTH ist es verboten, diese zu veröffentlichen.

Fehler gefunden?
----------------

Solltest du einen Fehler gefunden haben, füge diesen bitte dem Issue-Tracker hinzu.
