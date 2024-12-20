QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mode.cpp \
    protocol.cpp \
    receive.cpp \
    send.cpp \
    widget.cpp

HEADERS += \
    protocol.h \
    widget.h

FORMS += \
    widget.ui

RC_ICONS = icons/Elevator.ico

TRANSLATIONS += \
    Elevator_control_platform_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    icons/Elevator.ico \
    icons/down.png \
    icons/power_black.png \
    icons/power_green.png \
    icons/power_red.png \
    icons/stop.png \
    icons/up.png

RESOURCES += \
    resource.qrc
