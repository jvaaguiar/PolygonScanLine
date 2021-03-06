#-------------------------------------------------
#
# Project created by QtCreator 2019-03-28T11:19:54
#
#-------------------------------------------------

QT       += core gui opengl
LIBS     += -lopengl32

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = PolygonScanLine
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
    camera.cpp \
    cgutils.cpp \
    lightsource.cpp \
        main.cpp \
        mainwindow.cpp \
    canvasopengl.cpp \
    polygondrawer.cpp \
    drawer.cpp \
    linedrawer.cpp \
    mousefollower.cpp \
    hintboxdrawer.cpp \
    appcontroller.cpp \
    vertexholderdrawer.cpp \
    blocoet.cpp

HEADERS += \
    camera.h \
    cgutils.h \
    lightsource.h \
        mainwindow.h \
    canvasopengl.h \
    polygondrawer.h \
    drawer.h \
    linedrawer.h \
    mousefollower.h \
    hintboxdrawer.h \
    appcontroller.h \
    vertexholderdrawer.h \
    blocoet.h

FORMS += \
        mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
