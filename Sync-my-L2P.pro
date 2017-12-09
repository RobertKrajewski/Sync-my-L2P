QT     += core gui network xml
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++11

TARGET = Sync-my-L2P
TEMPLATE = app
DESTDIR = bin

SOURCES += \
    src/main.cpp \
    src/autoclosedialog.cpp \
    src/browser.cpp \
    src/filedownloader.cpp \
    src/info.cpp \
    src/l2pitemmodel.cpp \
    src/logger.cpp \
    src/login.cpp \
    src/logindialog.cpp \
    src/main.cpp \
    src/message.cpp \
    src/mymainwindow.cpp \
    src/mysortfilterproxymodel.cpp \
    src/options.cpp \
    src/parser.cpp \
    src/structureelement.cpp \
    src/utils.cpp

INCLUDEPATH += include/
HEADERS  += \
    include/clientId.h \
    include/autoclosedialog.h \
    include/browser.h \
    include/clientId.h \
    include/filedownloader.h \
    include/info.h \
    include/l2pitemmodel.h \
    include/logger.h \
    include/login.h \
    include/logindialog.h \
    include/message.h \
    include/mymainwindow.h \
    include/mysortfilterproxymodel.h \
    include/options.h \
    include/parser.h \
    include/structureelement.h \
    include/urls.h \
    include/utils.h

FORMS += \
    gui/autoclosedialog.ui \
    gui/browser.ui \
    gui/dateidownloader.ui \
    gui/info.ui \
    gui/logger.ui \
    gui/logindialog.ui \
    gui/message.ui \
    gui/mymainwindow.ui \
    gui/options.ui

TRANSLATIONS = lang/sync-my-l2p_de.ts \
               lang/sync-my-l2p_en.ts \
               lang/sync-my-l2p_lb.ts \
               lang/sync-my-l2p_sq.ts

RESOURCES += \
    icons\icons.qrc \
    lang\translation.qrc

RC_FILE = icon.rc

OTHER_FILES += \
    Sync-my-L2P.icns \
    README.md \
    magnifier.ico \
    LICENSE \
    .gitignore

include(qslog/QsLog.pri)
