#-------------------------------------------------
#
# Project created by QtCreator 2016-11-19T13:16:07
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SignalGUI
TEMPLATE = app
CONFIG += c++14

SOURCES += main.cpp\
        mainwindow.cpp \
    serialsession.cpp \
    measurementcontroller.cpp

HEADERS  += mainwindow.h \
    serialsession.h \
    measurementcontroller.h

FORMS    += mainwindow.ui
