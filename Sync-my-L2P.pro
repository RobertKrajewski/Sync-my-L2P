#-------------------------------------------------
#
# Project created by QtCreator 2012-10-09T09:25:30
#
#-------------------------------------------------

QT       += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Sync-my-L2P
TEMPLATE = app


SOURCES += main.cpp\
        mymainwindow.cpp \
    parser.cpp \
    structureelement.cpp \
    mysortfilterproxymodel.cpp \
    filedownloader.cpp \
    utils.cpp \
    browser.cpp \
    options.cpp \
    autoclosedialog.cpp \
    login.cpp \
    logindialog.cpp \
    qslog/QsLog.cpp \
    qslog/QsLogDest.cpp \
    qslog/QsLogDestConsole.cpp \
    qslog/QsLogDestFile.cpp \
    qslog/QsLogDestFunctor.cpp

HEADERS  += mymainwindow.h \
    parser.h \
    structureelement.h \
    mysortfilterproxymodel.h \
    filedownloader.h \
    utils.h \
    browser.h \
    options.h \
    autoclosedialog.h \
    login.h \
    clientId.h \
    logindialog.h \
    qslog/QsLog.h \
    qslog/QsLogDest.h \
    qslog/QsLogDestConsole.h \
    qslog/QsLogDestFile.h \
    qslog/QsLogDestFunctor.h \
    qslog/QsLogDisableForThisFile.h \
    qslog/QsLogLevel.h

FORMS    += mymainwindow.ui \
    mymainwindow.ui \
    dateidownloader.ui \
    browser.ui \
    options.ui \
    autoclosedialog.ui \
    logindialog.ui

OTHER_FILES += \
    Sync-my-L2P.icns \
    README.md \
    magnifier.ico \
    icon.rc \
    COPYING.LESSER \
    .gitignore

RESOURCES += \
    icons\icons.qrc
