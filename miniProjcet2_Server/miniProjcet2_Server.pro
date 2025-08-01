QT       += core gui network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

INCLUDEPATH += views networks models DB

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    models/clientHandler.cpp \
    main.cpp \
    views/mainwindow.cpp \
    networks/clientsetup.cpp \
    networks/servermanager.cpp \
    models/usermanage.cpp

HEADERS += \
    models/clientHandler.h \
    views/mainwindow.h \
    networks/clientsetup.h \
    networks/servermanager.h \
    models/usermanage.h

FORMS += \
    views/mainwindow.ui

TRANSLATIONS += \
    miniProjcet2_Server_ko_KR.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resorce.qrc
