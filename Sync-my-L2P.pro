QT += gui core network
SOURCES += \
    main.cpp \
    hauptfenster.cpp \
    strukturelement.cpp \
    datei.cpp \
    logintester.cpp \
    dateidownloader.cpp \
    mysortfilterproxymodel.cpp

HEADERS += \
    hauptfenster.h \
    strukturelement.h \
    datei.h \
    logintester.h \
    dateidownloader.h \
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


























































