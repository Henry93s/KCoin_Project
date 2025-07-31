QT       += core gui charts network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    data/candledata.cpp \
    data/linedata.cpp \
    main.cpp \
    mainwindow.cpp \
    models/filesendmanager.cpp \
    models/smtpemailsender.cpp \
    services/chartdatamanager.cpp \
    services/chatting_limiter.cpp \
    views/chattingRoomView.cpp \
    views/coinsearchlineedit.cpp \
    views/coinsearchwidget.cpp \
    views/homeview.cpp \
    views/linechartview.cpp \
    views/splashview.cpp \
    views/loginview.cpp \
    views/candlechart.cpp \
    views/chartstoolbox.cpp \
    views/signupview.cpp \
    views/dropwidget.cpp \
    models/mainmodel.cpp \
    models/candlechartdatamanager.cpp \
    models/chattinglistmanager.cpp \
    models/linechartdatamanager.cpp \
    models/sendingManage.cpp \
    models/socketManage.cpp \
    models/userManage.cpp \
    networks/socketnetwork.cpp

HEADERS += \
    data/candledata.h \
    data/linedata.h \
    mainwindow.h \
    models/filesendmanager.h \
    models/smtpemailsender.h \
    services/chartdatamanager.h \
    services/chatting_limiter.h \
    views/chattingRoomView.h \
    views/coinsearchlineedit.h \
    views/coinsearchwidget.h \
    views/homeview.h \
    views/linechartview.h \
    views/splashview.h \
    views/loginview.h \
    views/candlechart.h \
    views/chartstoolbox.h \
    views/dropwidget.h \
    views/signupview.h \
    models/candlechartdatamanager.h \
    models/mainmodel.h \
    models/chattinglistmanager.h \
    models/linechartdatamanager.h \
    models/sendingManage.h \
    models/socketManage.h \
    models/userManage.h

# FORMS += \
#     views/homeview.ui \
#     views/mainwindow.ui \
#     views/loginview.ui \
#     views/signupview.ui \
#     views/chattingRoomView.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc

ICON = assets/Bitcoin.icns
