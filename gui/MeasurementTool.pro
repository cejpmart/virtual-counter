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
DEFINES += QT_NO_PRINTER QT_NO_PRINTDIALOG QT_NO_PRINTPREVIEWWIDGET

QMAKE_CXXFLAGS += -Wno-unused

SOURCES += main.cpp\
        mainwindow.cpp \
    serialsession.cpp \
    measurementcontroller.cpp \
    qcustomplot/qcustomplot.cpp \
    pwmoutputplotcontroller.cpp

HEADERS  += mainwindow.h \
    serialsession.h \
    measurementcontroller.h \
    qcustomplot/qcustomplot.h \
    pwmoutputplotcontroller.h \
    guicommon.h
    ../common/protocoldefs.h

FORMS    += mainwindow.ui

# Deployment - Automatically Detect and Copy Dependencies to Build Folder

win32 {
    TARGET_CUSTOM_EXT = .exe
    DEPLOY_COMMAND = windeployqt

    CONFIG( debug, debug|release ) {
        # debug
        DEPLOY_TARGET = $$shell_quote($$shell_path($${OUT_PWD}/debug/$${TARGET}$${TARGET_CUSTOM_EXT}))
    } else {
        # release
        #DEPLOY_TARGET = $$shell_quote($$shell_path($${OUT_PWD}/release/$${TARGET}$${TARGET_CUSTOM_EXT}))
        DEPLOY_TARGET = $$shell_quote($${OUT_PWD}/release/$${TARGET}$${TARGET_CUSTOM_EXT})
    }

    # Uncomment the following line to help debug the deploy command when running qmake
    #warning($${DEPLOY_COMMAND} $${DEPLOY_TARGET})

    QMAKE_POST_LINK += $${DEPLOY_COMMAND} $${DEPLOY_TARGET}
}

QMAKE_POST_LINK += $$QMAKE_COPY $${PWD}/F042K6_Nucleo32.txt $${OUT_PWD}/

DISTFILES += \
    F042K6_Nucleo32.txt
