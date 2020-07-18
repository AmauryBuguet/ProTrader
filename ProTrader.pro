QT += core
QT += gui
QT += charts
QT += websockets
QT += testlib
QT += widgets

CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    apihandler.cpp \
    charthandler.cpp \
    globals.cpp \
    main.cpp \
    mainwindow.cpp \
    orderhandler.cpp \
    wshandler.cpp

HEADERS += \
    apihandler.hpp \
    charthandler.hpp \
    globals.hpp \
    mainwindow.hpp \
    orderhandler.hpp \
    wshandler.hpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Resources.qrc
