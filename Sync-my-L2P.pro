#-------------------------------------------------
#
# Project created by QtCreator 2012-10-09T09:25:30
#
#-------------------------------------------------

QT       += core gui network

TARGET = Sync-my-L2P
TEMPLATE = app


SOURCES += main.cpp\
        mymainwindow.cpp \
    parser.cpp \
    structureelement.cpp \
    mysortfilterproxymodel.cpp \
    logintester.cpp \
    filedownloader.cpp \
    utils.cpp \
    daterangedialog.cpp

HEADERS  += mymainwindow.h \
    parser.h \
    structureelement.h \
    mysortfilterproxymodel.h \
    logintester.h \
    filedownloader.h \
    utils.h \
    daterangedialog.h

FORMS    += mymainwindow.ui \
    mymainwindow.ui \
    logintester.ui \
    dateidownloader.ui \
    daterangedialog.ui

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
