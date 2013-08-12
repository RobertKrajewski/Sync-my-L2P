#-------------------------------------------------
#
# Project created by QtCreator 2012-10-09T09:25:30
#
#-------------------------------------------------

QT       += core gui network

TARGET = Sync-my-L2P
TEMPLATE = app

CONFIG += debug_and_release

SOURCES += main.cpp\
        mymainwindow.cpp \
    parser.cpp \
    structureelement.cpp \
    mysortfilterproxymodel.cpp \
    logintester.cpp \
    filedownloader.cpp \
    utils.cpp \
    browser.cpp \
    options.cpp \
    autoclosedialog.cpp

HEADERS  += mymainwindow.h \
    parser.h \
    structureelement.h \
    mysortfilterproxymodel.h \
    logintester.h \
    filedownloader.h \
    utils.h \
    browser.h \
    options.h \
    autoclosedialog.h

FORMS    += mymainwindow.ui \
    mymainwindow.ui \
    logintester.ui \
    dateidownloader.ui \
    browser.ui \
    options.ui \
    autoclosedialog.ui

OTHER_FILES += \
    Sync-my-L2P.icns \
    README.md \
    magnifier.ico \
    icon.rc \
    COPYING.LESSER \
    .gitignore

RESOURCES += \
    Icons.qrc \
    Certificates.qrc
