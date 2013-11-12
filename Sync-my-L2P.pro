QT += gui core network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
SOURCES += \
    main.cpp \
    mymainwindow.cpp \
    structureelement.cpp \
    myfile.cpp \
    logintester.cpp \
    filedownloader.cpp \
    mysortfilterproxymodel.cpp

HEADERS += \
    mymainwindow.h \
    structureelement.h \
    myfile.h \
    logintester.h \
    filedownloader.h \
    mysortfilterproxymodel.h

FORMS += \
    hauptfenster.ui \
    logintester.ui \
    dateidownloader.ui

RESOURCES += \
    Icons.qrc

RC_FILE += \
    ../Sync-my-L2P/icon.rc

ICON = Sync-my-L2P.icns

OTHER_FILES += \
    COPYING.LESSER


























































