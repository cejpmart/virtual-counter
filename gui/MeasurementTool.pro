#-------------------------------------------------
#
# Project created by QtCreator 2016-11-19T13:16:07
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MeasurementTool
TEMPLATE = app
CONFIG += c++14
INCLUDEPATH += qcustomplot
DEFINES += QT_NO_PRINTER

QMAKE_CXXFLAGS += -Wno-unused

SOURCES += main.cpp\
        mainwindow.cpp \
    serialsession.cpp \
    measurementcontroller.cpp \
    qcustomplot/qcustomplot.cpp

HEADERS  += mainwindow.h \
    serialsession.h \
    measurementcontroller.h \
    qcustomplot/qcustomplot.h
    ../common/protocoldefs.h

FORMS    += mainwindow.ui
